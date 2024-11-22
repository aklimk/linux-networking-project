#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include <csse2310_freeimage.h>
#include <FreeImage.h>
#include <csse2310a4.h>

#include "stringutils.h"
#include "ioutils.h"
#include "socketutils.h"
#include "httputils.h"

const char* const invalidServerCmdMessage
        = "Usage: uqimageproc [--max n] [--port port]\n";
const int invalidServerCmdCode = 14;

const char* const invalidServerPortFormat
        = "uqimageproc: unable to listen on port \"%s\"\n";
const int invalidServerPortCode = 19;

// Message formats for SIGHUP outputs.
const char* const connectedFormat = "Currently connected clients: %i\n";
const char* const completedFormat = "Completed clients: %i\n";
const char* const successfullFormat
        = "Successfully processed HTTP requests: %i\n";
const char* const erroredFormat = "HTTP requests unsuccessful: %i\n";
const char* const operationsFormat = "Operations on images completed: %i\n";

/* Thread shared statistics structure. Mutexes are spread over
 * reach struct field to reduce overall time spent waiting. */
typedef struct SharedStats {
    Mutex currentClients;
    Mutex finishedClients;
    Mutex okResponses;
    Mutex errorResponses;
    Mutex operationCompletions;
} SharedStats;

/* The data that a single thread should recieve wrapped in a void pointer */
typedef struct ThreadData {
    SharedStats* sharedStats;
    SocketData socketData;
} ThreadData;

/* handle_connection()
 * -------------------
 * Runtime logic for a single thread on the server. Each client interacts
 * with the server though this logic.
 *
 * data: a ThreadData pointer containing the shared statistics mutexes
 *      and informatin about the client socket.
 *
 * returns: NULL upon exit.
 *
 * REF: Semaphore handling based on race3.c example on week8 moss resources.
 */
void* handle_connection(void* data)
{
    // Recast void pointer to real type.
    ThreadData threadData = *((ThreadData*)data);
    SocketData socketData = threadData.socketData;
    free(data);
    HttpRequest inHttp = {0};
    modify_mutex(&(threadData.sharedStats->currentClients), 1);

    while (1) { // Loop until interuputed.
        free_array_of_headers(inHttp.headers);
        // Block until a http request is recieved on the input filestream.
        int error = !get_HTTP_request(socketData.get, &inHttp.type,
                &inHttp.address, &inHttp.headers, &inHttp.bodyData,
                &inHttp.bodyLen);

        // If HTTP requst is invalid, terminate the thread.
        if (error) {
            fclose(socketData.get);
            fclose(socketData.post);
            modify_mutex(&(threadData.sharedStats->finishedClients), 1);
            modify_mutex(&(threadData.sharedStats->currentClients), -1);
            return NULL;
        }

        // Respond to request forwarding the operation counter mutex.
        HttpResponse outHttp = respond_to_request(
                inHttp, &(threadData.sharedStats->operationCompletions));
        if (outHttp.status == HTTP_OK) {
            // Succesfful responses.
            modify_mutex(&(threadData.sharedStats->okResponses), 1);
        } else {
            // Error based responses.
            modify_mutex(&(threadData.sharedStats->errorResponses), 1);
        }

        // Construct HTTP response binary, writing to output filestream.
        long unsigned int responseLen;
        unsigned char* response = construct_HTTP_response(outHttp.status,
                outHttp.statusDescription, outHttp.headers, outHttp.bodyData,
                outHttp.bodyLen, &responseLen);
        fflush(stderr);
        fwrite(response, sizeof(unsigned char), responseLen, socketData.post);
        fflush(socketData.post);
        free(response);
    }
    modify_mutex(&(threadData.sharedStats->finishedClients), 1);
    modify_mutex(&(threadData.sharedStats->currentClients), -1);
    return NULL;
}

/* Data needed for a signal handling thread */
typedef struct SignalHandlerData {
    SharedStats* sharedStats;
    sigset_t* maskSet;
} SignalHandlerData;

/* signal_handler()
 * ----------------
 * Waits for a SIGHUP signal, at which point it prints the shared
 *      program statistics.
 *
 * data: a SignalHandlerData struct that holds a pointer to the
 *      shared statistics and a pointer to the mask set of the parent thread.
 *
 * returns: NULL when exited.
 *
 * REF: the following signal handling function is based off of the man page
 * REF: code example for pthread_sigmas(3).
 * REF: https://man7.org/linux/man-pages/man3/pthread_sigmask.3.html
 */
static void* signal_handler(void* data)
{
    // Recast void pointer as proper type.
    SignalHandlerData* sigData = (SignalHandlerData*)data;
    SharedStats* sharedStats = sigData->sharedStats;
    int signal;
    sigwait(sigData->maskSet, &signal);

    // Unlocks each semaphore individually to avoid blocking existing
    // threads for long periods of time.
    sem_wait(&(sharedStats->currentClients.lock));
    fprintf(stderr, connectedFormat, sharedStats->currentClients.value);
    sem_post(&(sharedStats->currentClients.lock));

    sem_wait(&(sharedStats->finishedClients.lock));
    fprintf(stderr, completedFormat, sharedStats->finishedClients.value);
    sem_post(&(sharedStats->finishedClients.lock));

    sem_wait(&(sharedStats->okResponses.lock));
    fprintf(stderr, successfullFormat, sharedStats->okResponses.value);
    sem_post(&(sharedStats->okResponses.lock));

    sem_wait(&(sharedStats->errorResponses.lock));
    fprintf(stderr, erroredFormat, sharedStats->errorResponses.value);
    sem_post(&(sharedStats->errorResponses.lock));

    sem_wait(&(sharedStats->operationCompletions.lock));
    fprintf(stderr, operationsFormat, sharedStats->operationCompletions.value);
    sem_post(&(sharedStats->operationCompletions.lock));
    fflush(stderr);
    return NULL;
}

/* initilize_shared_stats()
 * ------------------------
 * Private helper function that initilizes a SharedStats structure by
 *      initilizing all mutex semaphores.
 *
 * sharedStats: the SharedStats structure to initilize.
 *
 * REF: semaphore implementation based on week8 race3.c at moss.
 */
void initilize_shared_stats(SharedStats* sharedStats)
{
    sem_init(&(sharedStats->currentClients.lock), 0, 1);
    sem_init(&(sharedStats->finishedClients.lock), 0, 1);
    sem_init(&(sharedStats->okResponses.lock), 0, 1);
    sem_init(&(sharedStats->errorResponses.lock), 0, 1);
    sem_init(&(sharedStats->operationCompletions.lock), 0, 1);
}

/* Entry point for server application */
int main(int argc, char** argv)
{
    SharedStats sharedStats = {0};
    initilize_shared_stats(&sharedStats);

    // Parse and validate server command line arguments.
    ServerInputs args = parse_server_inputs(argc, argv);
    if (args.error) {
        fprintf(stderr, invalidServerCmdMessage);
        return invalidServerCmdCode;
    }
    if (!args.port) {
        args.port = "0"; // Use ephemeral port if non specified.
    }

    // Attempt to open the user supplied port for listening.
    int socketHandle = open_port(args.port);
    if (socketHandle == -1) {
        fprintf(stderr, invalidServerPortFormat, args.port);
        return invalidServerPortCode;
    }

    // REF: signal masking code is inspired by the man page
    // REF: code example for pthread_sigmask(3).
    // REF: https://man7.org/linux/man-pages/man3/pthread_sigmask.3.html
    // Allows the current and child threads to implement custom handling
    // for the SIGHUP signal.
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // Launch signal handler in a new thread.
    pthread_t sigHandlerID;
    SignalHandlerData sigHandlerData = {&sharedStats, &set};
    pthread_create(&sigHandlerID, NULL, signal_handler, &sigHandlerData);
    pthread_detach(sigHandlerID);
    while (1) {
        // Block the mean thread until a new connection is recieved.
        SocketData clientSocketData = block_for_connection(socketHandle);
        ThreadData threadData = {&sharedStats, clientSocketData};
        // REF: Thread handling inspired by moss
        // RED: week10 server-multithreaded example code.
        ThreadData* data = malloc(sizeof(ThreadData));
        *data = threadData;
        pthread_t threadID;
        pthread_create(&threadID, NULL, handle_connection, data);
        pthread_detach(threadID);
    }
    return 0;
}
