#include "zalewanie.h"
using namespace std;
/*
 * zmienne globalne
 */
 
 int okres; // zalewanie co ile dni (0 - codziennie, 1 - co drugi dzien itd.)
 int okres_temp;
 string tekst_maila;
 string temat_maila;
 string adres_maila;
 
 int main(int argc, char **argv){
	 
/*			
*  inicjalizacja
*/
     Ustawienia ust;
     if(ust.read() == 0){
		ust.wypisz();
	 }
	 else{
		cout << "Wyjście z programu z powodu bledu wczytania pliku z ustawieniami." << endl;
		cout << "Upewnij sie, ze plik z ustawieniami ustawienia.cfg jest poprawny." << endl;
		return -1;
	 }
		
	 okres = ust.get_okres();
	 okres_temp = 0;
	 bool running = 1; //warunek dzialania glownej petli
	 bool zalewaj = 0; //warunek zalewania
	 time_t rawtime;
	 time_t thistime = 0;
	 struct tm* czas;
	 
	 //Setup
	wiringPiSetup();          // inicjalizacja wiringPi
	pcf8574Setup(100, 0x27);  // inicjalizacja plytki z rozszerzeniem pinow
	int fd = wiringPiI2CSetup(0x4a); // inicjalizacja ADC do pomiaru temperatury
	if(fd == -1) {
		cout << "Brak polaczenia z ADC" << endl;
		cout << "Zamykam program." << endl;
		zapiszLog("Program zakonczony - brak polaczenia z ADC odczytu temperatury.");
		return -1;
	}
	
	Wire red_wire(104);    // zawor rurociagu
	Wire green_wire(105);  // zawor dolotowy zbiornika posredniego
	Wire blue_wire(106);   // zawor wylotowy zbiornika posredniego
	
	// RESET
	red_wire.set("OFF");
	green_wire.set("OFF");
	blue_wire.set("OFF");
	
	red_wire.printStatus();
	cout << endl;
	green_wire.printStatus(); 
	cout << endl;
	blue_wire.printStatus();
	cout << endl;
	
	cout << "Program zalewajacy wystartowal." << endl;
	zapiszLog("Program zalewajacy wystartowal. ");
				 
			 
	 //glowna petla
	 while(running){
		 time(&rawtime);
		 czas = localtime(&rawtime);
		 
		 if(czas->tm_sec == 0){
			  cout << "Automat zbiornika posredniego - czuwam: " << ctime(&rawtime) << endl;
			  printf("Chlodzenie rurociagu zaplanowane na godzine: %02d:%02d \n", ust.get_czas_chlodzenia_rur().godz, ust.get_czas_chlodzenia_rur().min);
			  printf("Zalewanie zaplanowane na godzine: %02d:%02d \n", ust.get_czas_zal().godz, ust.get_czas_zal().min);
			  cout << "do czasu zalewania dodaj " << okres_temp << " x 24h" << endl << endl;
		 } 
		 if(czas->tm_min == 0 && (czas->tm_sec == 0)){
			  zapiszLog("czuwam");
		 }
		 //zalewaj - warunek startu zalewania
		 zalewaj = (czas->tm_hour == ust.get_czas_chlodzenia_rur().godz)&&
		 (czas->tm_min == ust.get_czas_chlodzenia_rur().min)&&
		 (czas->tm_sec == 0)&&
		 (rawtime!=thistime);
		 
		 if(zalewaj){
			if(okres_temp==0){
				cout << "zalewam " << czas->tm_hour << " : " << czas->tm_min << " : " << czas->tm_sec << endl;
				
				if(0 == zalewanie(ust, red_wire, green_wire, blue_wire,fd)){
					sendmail(ust.get_adres_email(0).c_str(),
					"abraham@slcj.uw.edu.pl",
					"Zalewanie",
					"Wszystko OK");
				}
				
				time(&rawtime);
				czas = localtime(&rawtime);
				cout << "koniec zalewania " << czas->tm_hour << ":" << czas->tm_min << " : " << czas->tm_sec << endl;
				zapiszLog("koniec zalewania ");
				thistime = rawtime;
			}
			okres_temp--;
			if(okres_temp<0) okres_temp = okres;
			
			
		}
		 sleep(1);
	 }
 
 
 
 return 0;
 }

/*
 * funkcje 
 */
 
 int ads1110(int fd){
	 int rejestr = wiringPiI2CReadReg16 (fd, 0x9c);
	 int wynik = rejestr/256 + rejestr % 256 * 256;
	 return wynik;
	 
 }
 
 double napiecie(int fd){
	 double volty = 2.048*ads1110(fd)/32767;
	 return volty;
 }
 
 double opor (int fd){
	 double ohmy = napiecie(fd)*100.0 + 6.0;
	 return ohmy;
 }
 
 double temperatura(int fd){
	 //double celsius = 2.635 * opor(fd) - 261.8; //pierwsza kalibracja
	 double celsius = 2.475 * opor(fd) - 253.2;    //kalibracja z 30 sierpnia 2022
	 return celsius;
 }
 
int sendmail(const char *to, const char *from, const char *subject, const char *message)
{
    int retval = -1;
    FILE *mailpipe = popen("/usr/lib/sendmail -t", "w");
    if (mailpipe != NULL) {
        fprintf(mailpipe, "To: %s\n", to);
        fprintf(mailpipe, "From: %s\n", from);
        fprintf(mailpipe, "Subject: %s\n\n", subject);
        fwrite(message, 1, strlen(message), mailpipe);
        fwrite(".\n", 1, 2, mailpipe);
        pclose(mailpipe);
        retval = 0;
     }
     else {
         perror("Failed to invoke sendmail");
     }
     return retval;
}


bool przelewa(Ustawienia ust, int fd){
	double T = temperatura(fd);
	string temp = to_string(T);
	zapiszLog(temp);
	cout << "Temperatura przelewu: " << T << endl;
	if (T < ust.get_prog_przelewu()*1.0){
		cout << "Osiagnieto prog temperatury przelewu." << endl;
		zapiszLog("Osiagnieto prog temperatury przelewu.");
		return true;
	}
	else{
		return false;
	}
}

int zapiszLog(string text){
	time_t rawtime;
	FILE *dziennik;
	dziennik = fopen("./dziennik.log","ab");
	if (dziennik){
		const char * txt = text.c_str();
		time(&rawtime);
		fprintf(dziennik,"%.19s",ctime(&rawtime));
		fprintf(dziennik,"  ");	
		fprintf(dziennik,txt);
		fprintf(dziennik,"\n");
		fclose(dziennik);
	}
	
	
	return 0;
}

int zalewanie(Ustawienia ust,Wire zawor_rur, Wire dolotowy_zawor, Wire wylotowy_zawor, int fd){
	
	cout << "********** Poczatek nalewania" << endl;
	zapiszLog("********** Poczatek nalewania");
	int kod_bledu = 0;	
	zawor_rur.set("ON");
	cout << "Zawor rurociagu: ";
	zawor_rur.printStatus();
	cout << endl;
	zapiszLog(string("Zawor rurociagu: ") += zawor_rur.statusTxt());
	zapiszLog(string("Zawor dolotowy: ") += dolotowy_zawor.statusTxt());
	zapiszLog(string("Zawor wylotowy: ") += wylotowy_zawor.statusTxt());
	int min_chlodzenia =0;
	if(ust.get_czas_chlodzenia_rur().godz <= ust.get_czas_zal().godz){
		min_chlodzenia = ((ust.get_czas_zal().godz - ust.get_czas_chlodzenia_rur().godz) * 60);
		min_chlodzenia += ((ust.get_czas_zal().min - ust.get_czas_chlodzenia_rur().min));
		if(min_chlodzenia > 0){
			if(min_chlodzenia > 60){
				cout << "Czas chlodzenia rurociagu przekracza 60 minut." << endl;
				cout << "Ustawiam czas domyslny - 60 minut" << endl;
				zapiszLog("Ustawiony czas chlodzenia przekracza 60 min, chlodzenie bedzie trwalo 60 min");
				min_chlodzenia = 60;
			}
			cout << "Rozpoczynam chlodzenie rurociagu, ktore potrwa " << min_chlodzenia << " minut." << endl;
			zapiszLog(string("Chlodzenie rurociagu potrwa [minuty]: ") += to_string(min_chlodzenia));
			sleep(min_chlodzenia * 60);
		}
	}
	cout << "Zawor dolotowy: ";
	//dolotowy_zawor.set("ON"); // w tej konfiguracji musi byc wylaczony zeby sie napelnialo
	dolotowy_zawor.printStatus();
	cout << endl;
	
	cout << "Zawor wylotowy: ";
	wylotowy_zawor.set("ON");
	wylotowy_zawor.printStatus();
	cout << endl;
	
	for(int i = 0; i < (4*ust.get_min_nalewania()); i++){
		zapiszLog(string("Zawor rurociagu: ") += zawor_rur.statusTxt());
		zapiszLog(string("Zawor dolotowy: ") += dolotowy_zawor.statusTxt());
		zapiszLog(string("Zawor wylotowy: ") += wylotowy_zawor.statusTxt());	
	
		// sekcja podtrzymania stanu zaworow	
		cout << "Zawor rurociagu: ";
		zawor_rur.set("ON");
		zawor_rur.printStatus();	
		cout << endl;
	
		cout << "Zawor dolotowy: ";
		//dolotowy_zawor.set("ON"); // w tej konfiguracji musi byc wylaczony zeby sie napelnialo
		dolotowy_zawor.printStatus();
		cout << endl;
	
		cout << "Zawor wylotowy: ";
		wylotowy_zawor.set("ON");
		wylotowy_zawor.printStatus();
		cout << endl;
		// koniec sekcji podtrzymania stanu zaworow	
		sleep(15);
		if(przelewa(ust,fd) && (i > 7)) break; //sprawdza przelew po 2 min od startu
		if(i == (4*ust.get_min_nalewania()) - 1){
			cout << "Przekroczono czas nalewania" << endl;
			zapiszLog("Przekroczono czas nalewania");
			sendmail(ust.get_adres_email(0).c_str(),
				"abraham@slcj.uw.edu.pl",
				"Zalewanie",
				"Przekroczono czas nalewania");
				kod_bledu = -1;
		}
	}
	
	cout << "Zawor wylotowy: ";
	wylotowy_zawor.set("OFF");
	wylotowy_zawor.printStatus();
	cout << endl;
	
	sleep(60 * ust.get_min_dobijania());
	
	cout << "Zawor dolotowy: ";
	dolotowy_zawor.set("OFF");
	dolotowy_zawor.printStatus();
	cout << endl;
	
	cout << "Zawór rurociagu: ";
	zawor_rur.set("OFF");
	zawor_rur.printStatus();
	cout << endl;
	
	cout << "koniec nalewania" << endl;
	zapiszLog(string("Zawor rurociagu: ") += zawor_rur.statusTxt());
	zapiszLog(string("Zawor dolotowy: ") += dolotowy_zawor.statusTxt());
	zapiszLog(string("Zawor wylotowy: ") += wylotowy_zawor.statusTxt());
	return kod_bledu;
};

Wire::Wire(int i){
		this->id = i;
	};
	
int Wire::set(string status){
	if (status == "ON"){
		digitalWrite(this->id, LOW);
		return 0;
	}
	if (status == "OFF"){
		digitalWrite(this->id, HIGH);
		return 0;
	}
	return 1;
};
	
int Wire::status(void){
	int status; 
    status = digitalRead(this->id); 
	return status;		
};
	
int Wire::printStatus(void){
	int status; 
    status = digitalRead(this->id);
    if (status == 0){
			cout << "ON";
			return 0;
		}
		if (status == 1){
			cout << "OFF";
			return 0;
		}
	return 1;		
};

string Wire::statusTxt(void){
	int status; 
    status = digitalRead(this->id);
    if (status == 0){
			//cout << "ON";
			return "ON";
		}
		if (status == 1){
			//cout << "OFF";
			return "OFF";
		}
	return "false";		
};

int Wire::getId(void){
	return id;
};

int Godzina::set(int godz, int min){
	this->godz = godz;
	this->min = min;
	return 0;
};

Ustawienia::Ustawienia(void){
	nazwa_pliku="ustawienia.cfg";
	czas_zal.godz = 12;
	czas_zal.min = 20;
	okres = 0;
	prog_przelewu = -185;
	czas_chlodzenia_rur.godz = 12;
	czas_chlodzenia_rur.min = 0;
	min_dobijania = 5;
	min_nalewania = 180;
};

int Ustawienia::set_nazwa_pliku(string nazwa_pliku){
	this->nazwa_pliku = nazwa_pliku;
	return 0;
};

string Ustawienia::get_nazwa_pliku(void){
	return this->nazwa_pliku; 
};
string Ustawienia::get_adres_email(int nr){
	if (nr < 0 || nr > 2){
		cout << "nieprawidlowy nr adresu email ( nr prawidlowy z zakresu od 0 do 2)" << endl;
		return "null";
	}
	return this->adres_email[nr]; 
};

int Ustawienia::set_czas_zal(Godzina czas){
	this->czas_zal = czas;
	return 0;
};

int Ustawienia::save(void){
	FILE *plik;
	if(this->nazwa_pliku == "") 
			this->nazwa_pliku = "./settings.cfg";
	plik = fopen(this->nazwa_pliku.c_str(),"wb");
	if (plik){
		
		fprintf(plik,"Parametry nalewania: \n");
		fprintf(plik,"Godzina zalewania: %02d:%02d \n",this->czas_zal.godz, this->czas_zal.min);
		fprintf(plik,"Okres zalewania to 24h + n*24h, gdzie n = %d \n", this->okres);
		fprintf(plik,"Ustawienie progu reakcji sensora temperatury: %d \n", this->prog_przelewu);
		fprintf(plik,"Start chlodzenia rurociagu: %02d:%02d \n", this->czas_chlodzenia_rur.godz, this->czas_chlodzenia_rur.min);
		fprintf(plik,"Czas dobijania zbiornika: %d minut. \n", this->min_dobijania);
		fprintf(plik,"Czas dobijania zbiornika: %d minut. \n", this->min_nalewania);
		fprintf(plik,"Adres e-mail-1: %s;\nAdres e-mail-2: %s;\nAdres e-mail-3: %s;\n",
		this->adres_email[0].c_str(),this->adres_email[1].c_str(),this->adres_email[2].c_str());
		fclose(plik);
		cout << "*** zapisano ustawienia w " << this->nazwa_pliku << " ***" << endl;
	}
	
	return 0;
};

int Ustawienia::read(void){
	FILE *plik;
	const int bufor_size = 500;
	const int liczba_zmiennych = 20;
	int x[liczba_zmiennych];
	char bufor[bufor_size];
	
	
	if(this->nazwa_pliku == "") 
			this->nazwa_pliku = "./settings.cfg";
	plik = fopen(this->nazwa_pliku.c_str(),"r");
	if (plik){
		int lbufora = fread(bufor, sizeof(char), bufor_size, plik); //liczb znakow w pliku
		//cout << bufor << endl;
		bool was_number = 0;
		bool ujemna = 0;
		int k = 0;
			
		for(int i = 0; i < lbufora; i++){
			if(47 < bufor[i] && bufor[i] < 58) {
				if(was_number){
					if(ujemna) x[k] = (x[k])*10 - (((int)bufor[i]) - 48);
					else
					x[k] = (x[k])*10 + (((int)bufor[i]) - 48);
					}
					else{
						x[k] = ((int)bufor[i]) - 48;
						if(bufor[i-1] == 45) {
							ujemna = 1;
							x[k] = x[k]*(-1);
						}
						}
					was_number = 1;
				}
				else{
					if(was_number) k++;
					was_number = 0;
					ujemna = 0;
					}
		}
		//for(int j = 0; j < k; j++) cout << x[j] << endl;
		
		for(int i = 5; i < lbufora; i++){
			for(int n = 0; n < 3; n++){
				
				if(bufor[i-5] == 'm' &&
				   bufor[i-4] == 'a' &&
				   bufor[i-3] == 'i' &&
			       bufor[i-2] == 'l' &&
			       bufor[i-1] == '-' &&
			       bufor[i] == ('1'+n)){
					int j = i + 1;
					do{
						j++;
					}
					while(bufor[j] == 32);
				    while(bufor[j]!= 10 && bufor[j]!= 32 &&bufor[j]!= 59){
					   // 10 - ASCI koniec linii, 32 - ASCI spacja, 59 - ASCI srednik
					   adres_email[n] = adres_email[n] + bufor[j];
					   j++;
					   if (j == lbufora) break;
				    }
			   }
			}   
		}
		
		//---przypisuje zmienne z pliku do pol obiektu klasy Ustawienia
		this->czas_zal.godz = x[0];
		this->czas_zal.min = x[1];
		this->okres = x[4];
		this->prog_przelewu = x[5];
		this->czas_chlodzenia_rur.godz = x[6];
		this->czas_chlodzenia_rur.min = x[7];
		this->min_dobijania = x[8];
		this->min_nalewania = x[9];
		fclose(plik);
		cout << "*** wczytano ustawienia z " << this->nazwa_pliku << " ***" << endl;
	}
	else {
		cout << "ERROR - Nie odczytano pliku..." << endl;
		return -1;
	}
	
		
	return 0;
};

int Ustawienia::wypisz(void){
	printf("***** Wypisuje aktualne ustawienia programu *****\n");
	printf("Plik z ustawieniami: %s \n", this->nazwa_pliku.c_str());
	printf("Godzina zalewania: %02d:%02d \n",this->czas_zal.godz, this->czas_zal.min);
	printf("Okres zalewania to 24h + n*24h, gdzie n = %d \n", this->okres);
	printf("Ustawienie progu reakcji sensora temperatury: %d \n", this->prog_przelewu);
	printf("Start chlodzenia rurociagu: %02d:%02d \n", this->czas_chlodzenia_rur.godz, this->czas_chlodzenia_rur.min);
	printf("Czas dobijania zbiornika: %d minut. \n", this->min_dobijania);
	printf("Maksymalny czas nalewania zbiornika: %d minut. \n", this->min_nalewania);
	printf("Adres e-mail-1: %s;\nAdres e-mail-2: %s;\nAdres e-mail-3: %s;\n",
		this->adres_email[0].c_str(),this->adres_email[1].c_str(),this->adres_email[2].c_str());
	
	return 0;
};

Godzina Ustawienia::get_czas_zal(void){
	return this->czas_zal;
};

Godzina Ustawienia::get_czas_chlodzenia_rur(void){
	return this->czas_chlodzenia_rur;
};

int Ustawienia::get_okres(void){
	return this->okres;
};

int Ustawienia::get_min_dobijania(void){
	return this->min_dobijania;
};

int Ustawienia::get_min_nalewania(void){
	return this->min_nalewania;
};

int Ustawienia::get_prog_przelewu(void){
	return this->prog_przelewu;
};
