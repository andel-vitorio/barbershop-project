#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include <signal.h>
#include <semaphore.h>
#include <syncstream>
#include <chrono>
#include <queue>

#define CHAIRS_AMOUNT 2    // Número de cadeiras na sala de espera.

static int registersAmount = 0;  // Número de clientes[

#define debug_recruta printf

enum CostumerCategory {
	PAUSE,      // Pausa 
	OFFICER,    // Oficial
	SERGEANT,   // Sargento
	CORPORAL    // Cabo
};

enum BarberID {
	RECRUTA_ZERO,
	DENTINHO,   // Sargento
	OTTO    // Cabo
};

struct CustomerParams {
	int costumerCategory;
	int cutHairTimer;
	int id;

	CustomerParams() {
		this->costumerCategory = -1;
		this->cutHairTimer = -1;
		this->id = -1;
	}
};

int cost[] = { 3, 3, 3, 1, 1, 2 };

std::thread * btid;
std::vector<std::thread *> tid;

std::vector<std::thread *> customer, barber;

static int sleepingTimeTainha;

static sem_t customers;
static sem_t barbers;
static sem_t mutex;

int numberOfFreeSeats = CHAIRS_AMOUNT;   //Counter for Vacant seats in waiting room
int seatPocket[CHAIRS_AMOUNT];
static int barbAmount;               // Número de barbeiros
int sitHereNext = 0;                  //Index for next legitimate seat
int serveMeNext = 0;                  //Index to choose a candidate for cutting hair
static int count = 0;                 //Counter of No. of customers

std::vector<CustomerParams> registers;

std::queue<CustomerParams> officerQueue; 
std::queue<CustomerParams> sergeantQueue; 
std::queue<CustomerParams> corporalQueue;

static int nOfficer = 0;
static int nSergeant = 0;
static int nCorporal = 0;

bool finished_tainha = false;

// Representa o tipo de simulação da barbearia
static char caseID = 'A';

std::string getCustomerCategoryStr(int customerID) {
	if ( customerID == OFFICER ) {
		return "OFICIAL";
	} else if ( customerID == SERGEANT ) {
		return "SARGENTO";
	} else if ( customerID == CORPORAL ) {
		return "CABO";
	}

	return "";
}

void customerProcess(CustomerParams cp) {
	int costumerID = cp.costumerCategory;
	int mySeat;
	std::string costumerName;
	
	// Bloqueio da área mutua. Proteção das mudanças do assento.
	sem_wait(&mutex); {
		count++; // Chegada de clientes

		costumerID = cp.costumerCategory;

		if ( costumerID == OFFICER ) {
			costumerName = "OFICIAL";
			officerQueue.push(cp);
		} else if ( costumerID == SERGEANT ) {
			costumerName = "SARGENTO";
			sergeantQueue.push(cp);
		} else if ( costumerID == CORPORAL ) {
			costumerName = "CABO";
			corporalQueue.push(cp);
		}


		printf("Um %s [id=%d] entrou na sala de espera.\n", costumerName.c_str(), cp.id);
		sem_post(&mutex);                  // libera o bloqueio da área mútua
	}

	sem_post(&barbers);                // acorda um barberio
	sem_wait(&customers);              // entra na fila de cliente em espera

	printf("O %s [id=%d] deixou a barbearia.\n", costumerName.c_str(), cp.id);

	pthread_exit(0);
}

void barberEvent(int barberID) {   
	int myNext, hairCutTime;
	bool hasCustomer = false;
	CustomerParams cp;
	std::string barberName;

	if ( barberID == RECRUTA_ZERO ) barberName = "RECRUTA ZERO";
	else if ( barberID == DENTINHO ) barberName = "DENTINHO";
	else if ( barberID == OTTO ) barberName = "OTTO";
	
	printf("O barbeiro %s chegou na barbearia.\n", barberName.c_str());

	while (true) {   


		// Quando não há clientes, o barbeiro vai dormir.
		if ( officerQueue.empty() and sergeantQueue.empty() and corporalQueue.empty() ) {

			// Não mais cliente a serem adicionados
			if ( finished_tainha ) {
				printf("O barbeiro %s foi para casa.\n", barberName.c_str());
				return;
			}

			printf("O barbeiro %s foi dormir.\n", barberName.c_str());
			sem_wait(&barbers);
		}

		
		// Bloqueia um área exclusiva
		sem_wait(&mutex);  {
			
			if ( caseID == 'A' or caseID == 'B') {
				if ( !officerQueue.empty() ) {
					cp = officerQueue.front();
					officerQueue.pop();
				} else if ( !sergeantQueue.empty() )  {
					cp = sergeantQueue.front();
					sergeantQueue.pop();
				} else if ( !corporalQueue.empty() ) {
					cp = corporalQueue.front();
					corporalQueue.pop();
				}
			} 
			
			// Caso em que há 3 barbeiros trabalhando
			else if (caseID == 'C') {

				// Barbeiro Recruta Zero - responsável a atender os oficiais
				if ( barberID == RECRUTA_ZERO ) {
					
					// Se tiverem oficias na sala de espera, eles serão atendidos.
					if ( !officerQueue.empty() ) {
						cp = officerQueue.front();
						officerQueue.pop();
					} 
					
					// Caso não tenha um oficial, ou um sargente ou um cabo será atendido, se houver.
					else {

						// Caso tenha um sargento, ele será atendido
						if ( !sergeantQueue.empty() )  {
							cp = sergeantQueue.front();
							sergeantQueue.pop();
						}
						
						// Caso não tenha um sargento, mas tenha um cabo, ele será atendido
						else if ( !corporalQueue.empty() ) {
							cp = corporalQueue.front();
							corporalQueue.pop();
						}
					}
				}

				// Barbeiro Dentinho - responsável a atender os sargentos
				else if ( barberID == DENTINHO ) {

					// Se tiverem sargentos na sala de espera, eles serão atendidos.
					if ( !sergeantQueue.empty() )  {
						cp = sergeantQueue.front();
						sergeantQueue.pop();
					}
					
					// Caso não tenha um sargento, ou um oficial ou um cabo será atendido, se houver.
					else {

						// Caso tenha um oficial, ele será atendido
						if ( !officerQueue.empty() ) {
							cp = officerQueue.front();
							officerQueue.pop();
						} 
						
						// Caso não tenha um oficial, mas tenha um cabo, ele será atendido
						else if ( !corporalQueue.empty() ) {
							cp = corporalQueue.front();
							corporalQueue.pop();
						}
					}
				}

				// Barbeiro Dentinho - responsável a atender os cabos
				else if ( barberID == OTTO ) {
					
					// Se tiverem cabos na sala de espera, eles serão atendidos.
					if ( !corporalQueue.empty() ) {
						cp = corporalQueue.front();
						corporalQueue.pop();
					}
					
					//Caso não tenha um cabo, ou um oficial ou um sargento será atendido, se houver.
					else {

						// Caso tenha um oficial, ele será atendido
						if ( !officerQueue.empty() ) {
							cp = officerQueue.front();
							officerQueue.pop();
						} 
						
						// Caso não tenha um oficial, mas tenha um sargento, ele será atendido
						else if ( !sergeantQueue.empty() )  {
							cp = sergeantQueue.front();
							sergeantQueue.pop();
						}
					}

				}
			}

			sem_post(&mutex);
		}

		if ( cp.id == -1 )
			continue;		

		printf (
			"O barbeiro %s está cortando o cabelo de um %s [id=%d].\n", 
			barberName.c_str(), 
			getCustomerCategoryStr(cp.costumerCategory).c_str(), 
			cp.id
		);
		
		std::this_thread::sleep_for (std::chrono::seconds(cp.cutHairTimer));

		printf (
			"Barbeiro %s finalizou o corte de um %s [id=%d].\n", 
			barberName.c_str(),
			getCustomerCategoryStr(cp.costumerCategory).c_str(),
			cp.id
		);
		
		sem_post(&customers);
	}

	pthread_exit(0);
}

void tainha () {
	int costumerCategory;
	int i = 0;

	
	while ( i < registers.size() ) {

	  if ( officerQueue.size() + sergeantQueue.size() + corporalQueue.size() >= CHAIRS_AMOUNT ) {
		  continue;
	  }

	  costumerCategory = registers[i].costumerCategory;

	  if ( costumerCategory != PAUSE ) {
			std::thread * t = new std::thread(customerProcess, registers[i]);
			customer.push_back(t);
	  }
	
	  i++;


	  std::this_thread::sleep_for (std::chrono::seconds(sleepingTimeTainha));
	}

	printf("Sargento Tainha saiu para a sua casa.\n");
	finished_tainha = true;
}

int main (int argc, char *argv[]) {

	int category, cutHairTime;

	if ( argc == 3 ) {
		sleepingTimeTainha = atoi(argv[1]);
		caseID = argv[2][0];
	}

	if ( caseID == 'A') barbAmount = 1;
	else if ( caseID == 'B' ) barbAmount = 2;
	else if ( caseID == 'C' ) barbAmount = 3;

	srand(time(NULL));

	int id = 0;

	while (std::cin >> category >> cutHairTime) {
		CustomerParams cp;
		cp.costumerCategory = category;
		cp.cutHairTimer = cutHairTime;
		cp.id = id;

		registers.push_back(cp);
		id++;
	}

	registersAmount = registers.size();

	int i,status = 0;

	sem_init(&customers, 0, 0);
	sem_init(&barbers, 0, 0);
	sem_init(&mutex, 0, 1);

	printf("Barbearia está aberta\n");
	
	for ( i = 0; i < barbAmount; i++ ) {   
		std::thread * t = new std::thread(barberEvent, i);
		barber.push_back(t);
		status = 1;
		sleep(1);
	}

	std::thread * t = new std::thread(tainha);
	t->join();

	while ( !corporalQueue.empty() || !sergeantQueue.empty() || !officerQueue.empty() ); 

	for (int i = 0; i < barber.size(); i++)
		barber[i]->join();

	printf("Barbearia fechou!\n");


	exit(EXIT_SUCCESS);
}