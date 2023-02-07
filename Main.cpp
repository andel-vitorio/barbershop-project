#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include <signal.h>
#include <semaphore.h>
#include <syncstream>

#define CHAIRS_AMOUNT 3    // Número de cadeiras na sala de espera.
#define CUT_TIME 1          // Tempo de corte: 1s
#define BARD_AMOUNT 2       // Número de barbeiros
#define CUSTUMER_AMOUNT 10  // Número de clientes

std::thread * btid;
std::vector<std::thread *> tid;

static sem_t customers;
static sem_t barbers;
static sem_t mutex;

int numberOfFreeSeats = CHAIRS_AMOUNT;   //Counter for Vacant seats in waiting room
int seatPocket[CHAIRS_AMOUNT];           //To exchange pid between customer and barber
int sitHereNext = 0;                  //Index for next legitimate seat
int serveMeNext = 0;                  //Index to choose a candidate for cutting hair
static int count = 0;                 //Counter of No. of customers

void waitSecs(int secs) {
    int len;
    len = (int) ((1 * secs) + 1);
    sleep(len);
}

void customerProcess(int costumerID) {   
    int mySeat;
    
    sem_wait(&mutex);               // Bloqueio da área mutua. Proteção das mudanças do assento.
    count++;                        // Chegada de clientes

    printf("Cliente %d chegou na barbearia.\n", costumerID);
    
    if ( numberOfFreeSeats > 0 ) {
        --numberOfFreeSeats;        // Número de cadeiras restante na sala de espera

        printf("Cliente %d entrou na sala de espera.\n", costumerID);

        sitHereNext = (++sitHereNext) % CHAIRS_AMOUNT;  // Eschole a próxima cadeira vaga para se sentar
        mySeat = sitHereNext;

        seatPocket[mySeat] = costumerID;

        sem_post(&mutex);                  // libera o bloqueio da área mútua
        sem_post(&barbers);                // acorda um barberio

        sem_wait(&customers);              // entra na fila de cliente em espera

        sem_wait(&mutex);                  // bloqueia um área exclusiva
            numberOfFreeSeats++;                // sai da sala de espera e vai para a cadeira do barbeiro
        sem_post(&mutex);                  // libera uma área exclusica

        printf("Cliente %d deixou a barbearia.\n", costumerID);

    } else {
       sem_post(&mutex);  //Release the mutex and customer leaves without haircut
       printf("Cliente %d não encontrou lugar na sala de espera e saiu da barbearia.\n", costumerID);
    }

    pthread_exit(0);
}

void barberEvent(int barberID) {   
    int myNext, C;
    
    printf("Barbeiro %d chegou na barbearia.\n", barberID);

    while (true) {   
        printf("Barbeiro %d foi dormir.\n", barberID);

        sem_wait(&barbers);          // O barbeiro entra na fila dos barbeiros
        sem_wait(&mutex);            // Bloqueia um área exclusiva
          serveMeNext = (++serveMeNext) % CHAIRS_AMOUNT;  // escolhe o próximo cliente
          myNext = serveMeNext;
          C = seatPocket[myNext];                  //Get selected customer's PID
          seatPocket[myNext] = pthread_self();     //Leave own PID for customer
        sem_post(&mutex);
        sem_post(&customers);        
        printf("Barbeiro %d acordou e está cortando o cabelo do cliente %d.\n",barberID,C);
        sleep(CUT_TIME);
        printf("Barbeiro %d finalizou o corte.\n",barberID);
    }
}

void wait() {
     int x = rand() % (250000 - 50000 + 1) + 50000; 
     srand(time(NULL));
     usleep(x);
}

int main (void) {

    std::vector<std::thread *> barber,customer;
    int i,status = 0;

    sem_init(&customers, 0, 0);
    sem_init(&barbers, 0, 0);
    sem_init(&mutex, 0, 1);

    printf("Barbearia está aberta\n");
    
    for ( i = 0; i < BARD_AMOUNT; i++ ) {   
        std::thread * t = new std::thread(barberEvent, i);
        barber.push_back(t);
        status = 1;
        sleep(1);
    }

    for ( i = 0; i < CUSTUMER_AMOUNT; i++ ) {   
        std::thread * t = new std::thread(customerProcess, i);
        customer.push_back(t);
        status=1;
        wait();
    }

    for ( i = 0; i < CUSTUMER_AMOUNT; i++ )   
        customer[i]->join();

    printf("Barbearia fechou!\n");
    
    exit(EXIT_SUCCESS);
}