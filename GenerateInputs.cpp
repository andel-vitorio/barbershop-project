#include <iostream>

static const int n = 10;

enum CostumerCategory {
	PAUSE,      // Pausa 
	OFFICER,    // Oficial
	SERGEANT,   // Sargento
	CORPORAL    // Cabo
};

int main (void) {
    int category, cutHairTime;

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