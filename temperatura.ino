
// sensore
#include <dht.h>
dht DHT;
#define DHT22_PIN 4
//******************

// Display
#include <LiquidCrystal.h>
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

byte newChar[8] = {
        B00111,
        B00101,
        B00111,
        B00000,
        B00000,
        B00000,
        B00000,
        B00000
};

 // debug
// #define debug

 
 
#include <Process.h>
// #include <Console.h>


#define ON HIGH
#define OFF LOW
#define status_led 13
#define rele 2


unsigned long previousMillis = 0; // will store last time
unsigned long previousMillis1 = 0; // will store last time

// constants won't change :
const long interval = 60000;



String sito = "www.ilchetto.it";
Process date;

Process Send;
Process Canc;
Process Get;
//**********************



String settemp;
String setumid;
String setpoint = "";


bool togglesensore = false;
int umidita = 0;
float temperature = 0.0;
float umidity = 0.0;
String stato = "Spento";

// variabili x controllo max timeout or rebbot;

byte b = 0;


// inizializzo le variabli per il controllo del riscaldamneto
String Datastart = "";
String Dataend = "";
unsigned long oracorrente = 0;
unsigned long minuti = 0;
unsigned long tot_secnow = 0;
unsigned long tot_secstart = 0;
unsigned long  tot_secend = 0;
// setpoint
unsigned long setstartora = 1;
unsigned long setstartmin = 1;
unsigned long setendora = 1;
unsigned long setendmin = 1;

bool accensione = false;
int settemperatura = 1000;

bool automatic = true;
boolean currentState = LOW;//stroage for current button state
boolean lastState = LOW;//storage for last button state
#define modein 3


void setup() {
  pinMode(status_led, OUTPUT);
  pinMode(rele, OUTPUT);
  pinMode(modein, INPUT_PULLUP);
 
  digitalWrite(status_led, ON);
  digitalWrite(rele, OFF);
  
  
  lcd.createChar(1, newChar);
   lcd.begin(16, 2);
   
    
  lcd.setCursor(1, 0);
  lcd.print("Wait Startup!!!");
 
 
  Bridge.begin();
 


  delay (5000);

  if (!date.running())  {
	  date.begin("date");
	  date.addParameter("%D");
	  date.run();

  }

  digitalWrite(status_led, OFF);

  sito.trim();
  #ifdef debug
 Console.println("fine setup");
#endif

  
  sensor();
  delay(1000);
  getrss(); 
  delay(1000);
  controllo();
  delay(1000);
  lcd.clear();
}

// Loop*********************************************************************************************************************************************
void loop() {
	
	 

  unsigned long currentMillis = millis();

 
 currentState = digitalRead(modein);
  if (currentState == LOW && lastState == HIGH){//if button has just been pressed
   automatic = !automatic;   
  } 
    lastState = currentState;
	
	
	if (automatic == false)
	{
	settemperatura = map(analogRead(A0), 0, 1023, 1000, 2500);
	accensione = true;
	riscaldamento();
	}

else{
       
 

  if (currentMillis - previousMillis >= interval * 5.0) {

    previousMillis = currentMillis;
    getrss();
    controllo();

  }
  }
  if (currentMillis - previousMillis1 >= interval) {
    previousMillis1 = currentMillis;
	  sensor();
        send_temperatura();
    }
   



  // reboot check
  if (b >= 5) {
	  resetmcu();
  //****************
  }
  
  
  
  disp();
 
}
//***************************************************************************************************************************************************



// DATA E ORA

//****************************************************************************************************************************************************
// data corrente
String nowdata() {
  //************************************************************************************************************

  // metto a posto la data  per il DB
  date.begin("date");

  date.addParameter("+%F");
  date.run();
 

  String timeString;
  if (date.available() > 0) {
    timeString   = date.readString();
    

  }
  
 String Data  = timeString;


  date.close();
  Data.trim();
#ifdef debug
  Console.println(Data);

#endif


  //***********************
  return Data;
}



String nowtime() {
  //************************************************************************************************************

  // metto a posto  l'ora per il DB
  date.begin("date");

  date.addParameter("+%T");
  date.run();

  String timeString;
  if (date.available() > 0) {
    timeString   = date.readString();
    

  }

    
 String Data  = timeString;


  date.close();
  Data.trim();
#ifdef debug
  Console.println(Data);

#endif


  //***********************
  return Data;
}

// temperatura al DB  ********************************************************************************************************************************
void send_temperatura() {

  if (togglesensore  == true) {

#ifdef debug
    Console.println ("start Send");
#endif
    digitalWrite(status_led, ON);
  
    String datadb = nowdata();
    String timedb = nowtime();


    Send.begin("curl");  // Process that launch the "curl" command
    String Sitourl = sito + "/inserisci.php?data=" + datadb +"&ora="+ timedb + "&temperatura=" + settemp + "&umidita=" + setumid + "&stato=" + stato + "&setpoint=" + settemperatura / 100 ;
	Send.addParameter("--connect-timeout");
	Send.addParameter("10");
	Send.addParameter("-m");
	Send.addParameter("60");
	Send.addParameter( Sitourl ); // Add the URL parameter to "curl"
#ifdef debug
    Console.println (Sitourl);
#endif
    Send.run();// Run the process and wait for its termination
  

    String risp = "";
#ifdef debug
    Console.println ("send run");
    Console.println(Send.exitValue());
#endif

    if (Send.exitValue() == 0 && Send.available() > 0) {

      risp = Send.readString();
      Send.close();
	   b = 0 ;
	 
	 

    }

    else
    {
#ifdef debug
      Console.println("send timeout");
#endif
      b++;
      Send.close();
	  errore();
	  return;
    }

    risp.trim();
#ifdef debug
    Console.println (risp);
#endif

    if (risp == "dati inseriti") {


      digitalWrite(status_led, OFF);
    }

#ifdef debug
    Console.println ("end send");
#endif


  }
}

// Sensori *******************************************************************************************************************************************
void sensor()
{



  int chk = DHT.read22(DHT22_PIN);

  if ( chk == 0) {
    umidity = DHT.humidity;
    temperature = DHT.temperature;

    togglesensore = true;
	  settemp = String ( temperature, DEC);
    setumid = String (umidity, DEC);

    
}

  else {

    togglesensore = false;
#ifdef debug
	Console.println("Errore Sensore");
#endif


  }


}
// Rss *****************************************************************************************************************************************
void getrss()
{

#ifdef debug
  Console.println ("start rss");
#endif

  setpoint = "";
  digitalWrite(status_led, ON) ;


#ifdef debug
  Console.println ("start rss process");
#endif
  Get.begin("curl");  // Process that launch the "curl" command
  Get.addParameter("--connect-timeout");
  Get.addParameter("10");
  Get.addParameter("-m");
  Get.addParameter("60");
  Get.addParameter(sito + "/Calendario.php"); // Add the URL parameter to "curl"

  Get.run();

 

  if (Get.exitValue() == 0 && Get.available() > 0) {

    setpoint = Get.readString();
    Get.close();
	b = 0 ;
  }
  else
  {
    	setpoint ="Errore";
#ifdef debug
    Console.println("rss timeout");

#endif

    Get.close();
    b++;
	errore();
   
	return;
  }


  setpoint.trim();
#ifdef debug

#endif

  digitalWrite(status_led, OFF);
#ifdef debug
  Console.println ("end rss ");

#endif

}
//****************************************************************************************************************************************************

// controllo *****************************************************************************************************************************************
void controllo()
{


#ifdef debug
  Console.println ("start controllo");
  Console.println (setpoint.length());
#endif

     String now = "";
     String data = "";
     
   now = nowtime();
   data = nowdata();
   oracorrente = now.substring(0, 2).toInt();
   minuti = now.substring(3, 5).toInt();
   tot_secnow = oracorrente * 3600 + minuti * 60;
 
  if ( setpoint.length() == 80) {

    // setpoint
   Datastart = setpoint.substring(24, 34);
   Dataend = setpoint.substring(55, 65);
    Datastart.trim();
	Dataend.trim();
   
    // inizializzo a intero le ore e i minuti del setpoint
    setstartora = setpoint.substring(35, 37).toInt();
    setstartmin = setpoint.substring(38, 40).toInt();

    setendora = setpoint.substring(66, 68).toInt();
    setendmin = setpoint.substring(69, 71).toInt();


    

    tot_secstart =  setstartora * 3600 +  setstartmin * 60;
    tot_secend = setendora * 3600 +  setendmin * 60;
    settemperatura = setpoint.substring(14, 16).toInt() * 100;
    }
    if ((data == Datastart && data == Dataend) &&  ((tot_secnow >= tot_secstart) && (tot_secnow <= tot_secend))) {
      accensione = true;
      
    }
	else if ((data == Datastart && data != Dataend ) &&  ((tot_secnow >= tot_secstart) )) {
      accensione = true;
     
    }
	else if ((data != Datastart && data == Dataend ) &&  ((tot_secnow <= tot_secend) )) {
      accensione = true;
       
    }

    else {
      accensione = false;
       settemperatura = 1000; 
    }


  
 
  riscaldamento();

#ifdef debug

  Console.println ("end controllo");
#endif

}
//****************************************************************************************************************************************************

// riscaldamento *************************************************************************************************************************************
void riscaldamento()
{
  if (togglesensore == true)
  {

    int temperatura = temperature * 100;

    if (accensione == true) {
	
      int tolleranceplus = settemperatura ;
      int tolleranceminus = settemperatura - 50;



      if (  temperatura <= tolleranceminus)
      {

        stato = "Acceso";


        digitalWrite(rele, ON);

      }

      else if ( temperatura >= tolleranceplus)
      {
        digitalWrite(rele, OFF);

        stato = "Spento";


      }

    }
    else { // temperatura minima di 10 gradi
      int mintemperature = 1000;
      settemperatura = mintemperature;
      if (temperatura <= mintemperature) {

        stato = "Acceso"; 


        digitalWrite(rele, ON);

      }
      else {


        stato = "Spento";

        digitalWrite(rele, OFF);


      }

    }

  }


 
#ifdef debug

  Console.println (stato);
#endif
}
//****************************************************************************************************************************************************



void errore()
{

	
	 lcd.setCursor(2,1);
	lcd.print(F("errore"));
	delay(2000);
        lcd.setCursor(2,1);
        lcd.print (stato);
}

//*********************************************

void resetmcu() {
  Process reboot;

  //  Console.println("reset");
  //delay (1000);
  //reboot.runShellCommand ("/usr/bin/blink-start 50");
  //delay (2000);
  //reboot.runShellCommand ("/usr/bin/blink-stop");
 // delay (1000);
  reboot.runShellCommand("/usr/bin/reboot");
 
}

void disp()
{
     
    lcd.setCursor(2, 0);
    lcd.print( settemp.substring(0, 4));
    lcd.write(1);
    lcd.print  ("C " + setumid.substring(0, 4) +"%");
    lcd.setCursor(2, 1);
    lcd.print (stato);
    if (automatic == true){
   lcd.print ( F(" A "));
   }
   
   else {
     lcd.print (F( " M "));
     }
     
       lcd.print ( settemperatura / 100);
       
      
   
    lcd.write(1);
    lcd.print(F("C"));
}

