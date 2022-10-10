#ifndef ZALEWANIE_H
#define ZALEWANIE_H

#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <ctime>
#include <string>
#include <string.h>
#include "unistd.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pcf8574.h>


//klasy

class Godzina{
	 public:
	 int godz;
	 int min;
	 int set(int godz,int min);
 };

class Ustawienia{
	private:
	std::string nazwa_pliku="";  // nazwa pliku z ustawieniami
	Godzina czas_zal;            // godzina rozpoczecia nalepniania zbiornika 
	int okres;                   // Okres zalewania to 24h + n*24h, gdzie n = okres (domyslnie 0 dla zalewania codziennego)
	int prog_przelewu;           // wartosc z czujnika przelewu zamykajaca wylew
	Godzina czas_chlodzenia_rur; // godzina o ktorej startuje chlodzenie rurociagu
	int min_dobijania;          // czas po zamknieciu zaworu odpowietrzajacego kiedy jeszcze zbiornik jest "dobijany" (w minutach)
	int min_nalewania;			 // maksymalny przewidziny czas nalewania w minutach
	std::string adres_email[3];  // adresy e-mail do wysylania raportow (max 3)
	
	public:
	Ustawienia();
	//~Ustawienia();
	int save(void);
	int read(void);
	int wypisz(void);
	int set_nazwa_pliku(std::string nazwa_pliku);
	std::string get_nazwa_pliku(void);
	std::string get_adres_email(int nr);
	int set_czas_zal(Godzina czas);
	Godzina get_czas_zal();
	Godzina get_czas_chlodzenia_rur();
	int get_okres();
	int get_min_dobijania();
	int get_min_nalewania();
	int get_prog_przelewu();
	
};

class Wire{
	private: //pola
	
	int id;
	 
	public: //metody
	
	Wire(int i);	
	int set(std::string status); 
	int status(void);
	int printStatus(void);
	std::string statusTxt(void);
	int getId(void);
};

// funkcje

int sendmail(const char *to,
 const char *from,
 const char *subject,
 const char *message);

int ads1110(int fd);
double napiecie(int fd);
double opor (int fd);
double temperatura(int fd);
int czekaj_sek(int sekundy);
int zalewanie(Ustawienia ust, Wire zawor_rur, Wire dolotowy_zawor, Wire wylotowy_zawor, int fd);
bool przelewa(Ustawienia ust, int fd);
int zapiszLog(std::string text);


#endif
