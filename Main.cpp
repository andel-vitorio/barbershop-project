#include <iostream>
#include <unistd.h>
#include <thread>
#include <vector>
#include <signal.h>
#include <semaphore.h>
#include <syncstream>
#include <chrono>
#include <queue>

#define CHAIRS_AMOUNT 20    // Número de cadeiras na sala de espera.

static int registersAmount = 0;  // Número de clientes[

#define debug // printf
#define show printf

class Metrics {

	public:
	
	double officer, sergeant, corporal, empty;
	
	Metrics(double officer, double sergeant, double corporal) {
		this->empty = CHAIRS_AMOUNT - officer -  sergeant - corporal;
		this->officer = officer;
		this->sergeant = sergeant;
		this->corporal = corporal;
	}

	Metrics() {
		this->empty = 0;
		this->officer = 0;
		this->sergeant = 0;
		this->corporal = 0;
	}

	void operator += (const Metrics & os) {
		this->officer += os.officer;
		this->sergeant += os.sergeant;
		this->corporal += os.corporal;
		this->empty += os.empty;
	}

	void operator /= (const double & num) {
		this->officer /= num;
		this->sergeant /= num;
		this->corporal /= num;
		this->empty /= num;
	}

	Metrics operator / (const double & num) {
		return Metrics(
			this->officer / num,
			this->sergeant / num,
			this->corporal / num
		);
	}
};

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
	int category;
	int serviceTime;
	int id;

	std::chrono::_V2::system_clock::time_point startTime;
	std::chrono::_V2::system_clock::time_point endTime;
	std::chrono::duration<double> elapsedTime;

	CustomerParams() {
		this->category = -1;
		this->serviceTime = -1;
		this->id = -1;
	}
};

int cost[] = { 3, 3, 3, 1, 1, 2 };

std::thread * btid;
std::vector<std::thread *> tid;

std::vector<std::thread *> customer, barber;

static int tainhaSleepingTime;

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

static bool barberShopIsClosed = false;
int nOfficer = 0;
int nSergeant = 0;
int nCorporal = 0;
int nBreak = 0;

bool tainhaIsFinished = false;

Metrics totalServiceTime;
Metrics totalWaitingTime;
Metrics servicesAmount;

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
	cp.startTime = std::chrono::system_clock::now();

	int costumerID = cp.category;
	int mySeat;
	std::string costumerName;
	
	// Bloqueio da área mutua. Proteção das mudanças do assento.
	sem_wait(&mutex); {
		count++; // Chegada de clientes

		costumerID = cp.category;

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


		debug("Um %s [id=%d] entrou na sala de espera.\n", costumerName.c_str(), cp.id);
		sem_post(&mutex);                  // libera o bloqueio da área mútua
	}

	
	sem_post(&barbers);                // acorda um barberio
	sem_wait(&customers);              // entra na fila de cliente em espera
	
	debug("O %s [id=%d] deixou a barbearia.\n", costumerName.c_str(), cp.id);

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
	
	debug("O barbeiro %s chegou na barbearia.\n", barberName.c_str());

	while (true) {   


		// Quando não há clientes, o barbeiro vai dormir.
		if ( officerQueue.empty() and sergeantQueue.empty() and corporalQueue.empty() ) {

			// Não mais cliente a serem adicionados
			if ( tainhaIsFinished ) {
				debug("O barbeiro %s foi para casa.\n", barberName.c_str());
				return;
			}

			debug("O barbeiro %s foi dormir.\n", barberName.c_str());
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

		cp.endTime = std::chrono::system_clock::now();
		cp.elapsedTime = cp.endTime - cp.startTime;

		if ( cp.category == OFFICER ) {
			totalWaitingTime.officer += (cp.elapsedTime.count());
			totalServiceTime.officer += (double) cp.serviceTime;
			servicesAmount.officer += 1.0;
		} else if ( cp.category == SERGEANT ) {
			totalWaitingTime.sergeant += (cp.elapsedTime.count());
			totalServiceTime.sergeant += (double) cp.serviceTime;
			servicesAmount.sergeant += 1.0;
		}	else if ( cp.category == CORPORAL ) {
			totalWaitingTime.corporal += (cp.elapsedTime.count());
			totalServiceTime.corporal += (double) cp.serviceTime;
			servicesAmount.corporal += 1.0;
		}

		debug (
			"O barbeiro %s está cortando o cabelo de um %s [id=%d].\n", 
			barberName.c_str(), 
			getCustomerCategoryStr(cp.category).c_str(), 
			cp.id
		);
		
		std::this_thread::sleep_for (std::chrono::seconds(cp.serviceTime));

		debug (
			"Barbeiro %s finalizou o corte de um %s [id=%d].\n", 
			barberName.c_str(),
			getCustomerCategoryStr(cp.category).c_str(),
			cp.id
		);
		
		sem_post(&customers);
	}

	pthread_exit(0);
}

void tainha () {
	int costumerCategory;
	int nZeros = 0;
	int i = 0;

	
	while ( i < registers.size() and nZeros != 3 ) {

	  std::this_thread::sleep_for (std::chrono::seconds(tainhaSleepingTime));

		if ( registers[i].category == OFFICER ) nOfficer++;
		else if ( registers[i].category == SERGEANT ) nSergeant++;
		else if ( registers[i].category == CORPORAL ) nCorporal++;
		else if ( registers[i].category == PAUSE ) nBreak++;

	  if ( officerQueue.size() + sergeantQueue.size() + corporalQueue.size() >= CHAIRS_AMOUNT ) {
			i++;
			continue;
	  }


	  costumerCategory = registers[i].category;

	  if ( costumerCategory != PAUSE ) {
			std::thread * t = new std::thread(customerProcess, registers[i]);
			customer.push_back(t);
			nZeros = 0;
	  } else nZeros++;

		i++;
	}

	debug("Sargento Tainha saiu para a sua casa.\n");
	tainhaIsFinished = true;
}

void escovinha () {
	Metrics totalOcupationState;
	Metrics totalQueueSizes;
	int readsAmount = 0;

	while ( !barberShopIsClosed ) {

		readsAmount++;

		Metrics currOcupationState((double) officerQueue.size(), (double) sergeantQueue.size(), (double) corporalQueue.size());
		Metrics queueSizes((double) officerQueue.size(), (double) sergeantQueue.size(), (double) corporalQueue.size());

		currOcupationState /= (double) CHAIRS_AMOUNT;

		debug (
			"\n\nEstado de Ocupação:\nOficial: %.2lf%%\nSargento: %.2lf%%\nCabo: %.2lf%%\nVazio: %.2lf%%\n\n", 
			currOcupationState.officer * 100.0, 
			currOcupationState.sergeant * 100.0, 
			currOcupationState.corporal * 100.0, 
			currOcupationState.empty * 100.0
		);

		totalOcupationState += currOcupationState;
		totalQueueSizes += queueSizes;
		
		std::this_thread::sleep_for (std::chrono::seconds(3));
	}

	totalOcupationState /= (double) readsAmount;
	totalQueueSizes /= (double) readsAmount;

	show ( "\n\nQuantidade de Leituras Feitas pelo Escovinha: %d\n\n",	readsAmount );

	show (
		"\n\nEstado de Ocupação das Cadeiras em Média:\nOficial: %.2lf%%\nSargento: %.2lf%%\nCabo: %.2lf%%\nVazio: %.2lf%%\n\n", 
		totalOcupationState.officer * 100.0, 
		totalOcupationState.sergeant * 100.0, 
		totalOcupationState.corporal * 100.0, 
		totalOcupationState.empty * 100.0
	);

	show (
		"Comprimento Médio da Filas:\nOficial: %.2lf\nSargento: %.2lf\nCabo: %.2lf\n\n", 
		totalQueueSizes.officer, 
		totalQueueSizes.sergeant, 
		totalQueueSizes.corporal
	);

	show (
		"Tempo Médio de Atendimento por Categoria:\nOficial: %.2lf\nSargento: %.2lf\nCabo: %.2lf\n\n", 
		( servicesAmount.officer != 0.0 ) ? ( totalServiceTime.officer / servicesAmount.officer ) : 0.0, 
		( servicesAmount.sergeant != 0.0 ) ? ( totalServiceTime.sergeant / servicesAmount.sergeant ) : 0.0, 
		( servicesAmount.corporal != 0.0 ) ? ( totalServiceTime.corporal / servicesAmount.corporal ) : 0.0
	);

	show (
		"Tempo Médio de Espera por Categoria:\nOficial: %.2lf s\nSargento: %.2lf s\nCabo: %.2lf s\n\n", 
		( servicesAmount.officer != 0.0 ) ? ( totalWaitingTime.officer / servicesAmount.officer ) : 0.0, 
		( servicesAmount.sergeant != 0.0 ) ? ( totalWaitingTime.sergeant / servicesAmount.sergeant ) : 0.0, 
		( servicesAmount.corporal != 0.0 ) ? ( totalWaitingTime.corporal / servicesAmount.corporal ) : 0.0
	);
	
	show (
		"Número de Atendimentos por Categoria:\nOficial: %.2lf\nSargento: %.2lf\nCabo: %.2lf\n\n", 
		servicesAmount.officer, 
		servicesAmount.sergeant, 
		servicesAmount.corporal
	);

	show (
		"Numero total de cliente(s) por categoria:\nOficial(is): %d\nSargento(s): %d\nCabo(s): %d\nPausa(s): %d\n\n", 
		nOfficer, nSergeant, nCorporal, nBreak
	);
}

int main (int argc, char *argv[]) {

	int category, cutHairTime;

	if ( argc == 3 ) {
		tainhaSleepingTime = atoi(argv[1]);
		caseID = argv[2][0];
	}

	if ( caseID == 'A') barbAmount = 1;
	else if ( caseID == 'B' ) barbAmount = 2;
	else if ( caseID == 'C' ) barbAmount = 3;

	srand(time(NULL));

	int id = 0;

	while (std::cin >> category >> cutHairTime) {
		CustomerParams cp;
		cp.category = category;
		cp.serviceTime = cutHairTime;
		cp.id = id;

		registers.push_back(cp);
		id++;
	}

	registersAmount = registers.size();

	int i,status = 0;

	sem_init(&customers, 0, 0);
	sem_init(&barbers, 0, 0);
	sem_init(&mutex, 0, 1);

	debug("Barbearia está aberta\n");
	
	for ( i = 0; i < barbAmount; i++ ) {   
		std::thread * t = new std::thread(barberEvent, i);
		barber.push_back(t);
		status = 1;
		sleep(1);
	}

	std::thread * escovinhaThread = new std::thread(escovinha);
	std::thread * t = new std::thread(tainha);
	t->join();

	for (int i = 0; i < customer.size(); i++)
		customer[i]->join();

	for (int i = 0; i < barber.size(); i++)
		pthread_cancel((pthread_t) barber[i]);


	debug("Barbearia fechou!\n");
	barberShopIsClosed = true;
	escovinhaThread->join();

	debug("Relatório Final\n");




	exit(EXIT_SUCCESS);
}