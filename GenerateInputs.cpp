#include <iostream>

enum CostumerCategory {
	PAUSE,      // Pausa 
	OFFICER,    // Oficial
	SERGEANT,   // Sargento
	CORPORAL    // Cabo
};

int main (int argc, char *argv[]) {
    int category, cutHairTime, n;

    if ( argc == 2 ) {
		n = atoi(argv[1]);
	}

	srand(time(NULL));

    for ( int i = 0; i < n; ++i ) {
        
        int category = rand() % 4;

        if ( category == OFFICER )
			cutHairTime = 4 + rand() % 3;
		else if ( category == SERGEANT )
			cutHairTime = 2 + rand() % 3;
		else if ( category == CORPORAL )
			cutHairTime = 1 + rand() % 3;
		else cutHairTime = 0;

        std::cout << category << ' ' << cutHairTime << '\n';
    }

    return 0;
}