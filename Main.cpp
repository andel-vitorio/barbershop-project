#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include <signal.h>
#include <semaphore.h>
#include <syncstream>

std::thread * btid;
std::vector<std::thread *> tid;

static sem_t waitingRoom;
static sem_t barberChair;
static sem_t barberPillow;
static sem_t seatBelt;

static int allDone = 0;

void waitSecs(int secs) {
    int len;
    len = (int) ((1 * secs) + 1);
    sleep(len);
}

void customer (int num) {
    // Chegada do cliente
    std::osyncstream(std::cout) << "Cliente " << num << " chegou na barbearia.\n";

    // Espera um lugar livre na sala de espera
    sem_wait(&waitingRoom);
    std::osyncstream(std::cout) << "Cliente " << num << " entrou na sala de espera.\n";

    // Espera a cadeira do barbeiro ficar livre
    sem_wait(&barberChair);

    // Sai da sala de espera quando a cadeira do barbeira estiver livre
    sem_post(&waitingRoom);

    // O cliente acorda do barbeiro
    std::osyncstream(std::cout) << "Cliente " << num << " acordou o barbeiro.\n";
    sem_post(&barberPillow);

    // Espera o barbeiro finalizar o corte
    sem_wait(&seatBelt);

    // O cliente sai da barbearia
    sem_post(&barberChair);
    std::osyncstream(std::cout) << "Cliente " << num << " saiu da barbearia.\n";
}

void barber () {
    
    while (!allDone) {

        // O barbeiro estará dormindo até que alguém o acorde
        std::osyncstream(std::cout) << "O barbeiro está dormindo\n";
        sem_wait(&barberPillow);

        if (!allDone) {
            
            std::osyncstream(std::cout) << "O barbeiro está cortando cabelo\n";
            waitSecs(2);
            std::osyncstream(std::cout) << "O barbeiro finalizou o corte de cabelo.\n";

            // O barbeiro avisa ao cliente que o corte foi finalizado
            sem_post(&seatBelt);
        } else {
            std::osyncstream(std::cout) << "O barbeiro está indo para casa.\n";
        }
    }
}

int main (void) {
    int numCostumers = 10, numChairs = 2;


    if (sem_init(&waitingRoom, 0, numChairs) == -1)
        std::cerr << "Error: waitingRoom" << '\n';
    
    if (sem_init(&barberChair, 0, 1) == -1)
        std::cerr << "Error: barberChair" << '\n';

    if (sem_init(&barberPillow, 0, 0) == -1)
        std::cerr << "Error: barberPillow" << '\n';

    if (sem_init(&seatBelt, 0, 0) == -1)
        std::cerr << "Error: seatBelt" << '\n';

   btid = new std::thread(barber);

    for (int i = 0; i < numCostumers; ++i) {
        std::thread * t = new std::thread(customer, i);
        tid.push_back(t);
    }

    for (int i = 0; i < tid.size(); ++i)
        tid[i]->join();

    allDone = 1;
    sem_post(&barberPillow);
    btid->join();

    return EXIT_SUCCESS;
}