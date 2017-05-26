///TRUC A FAIRE
/*
* déplacer le bouton d'arret d'urgance en haut de la CATAPUUUULTE
* ajouter des capteurs
* +ecran lcd
* reprendre le code de la fonction de callback pour mieux gérer les multiple capteurs
*/

///INCLUDES///
#include <NewPing.h>
//#include <digitalWriteFast.h>
#include <Stepper.h>
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include "rgb_lcd.h"
#include "Timer.h"
#include <QueueArray.h>

///PORTS CONNECTES///
//Ultrasons//
#define trigPin1 17
#define echoPin1 16
#define trigPin2 19
#define echoPin2 18
#define trigPin3 22
#define echoPin3 23

//Moteurs pas à pas//
#define moteurDENA 37
#define moteurDENB 36
#define moteurDIN1 10
#define moteurDIN2 11
#define moteurDIN3 12
#define moteurDIN4 13

#define moteurGENA 34
#define moteurGENB 35
#define moteurGIN1 6
#define moteurGIN2 7
#define moteurGIN3 8
#define moteurGIN4 9

//Servomoteurs//
#define servoFunny 30

//Ecran LCD RGB//
#define SDALCD 20
#define SCLLCD 21

//Com SPI Rasp//
#define MOSI 50
#define MISO 51

//Attente tirette//
#define tirette 24

///CONSTANTES///
//Moteurs pas à pas//
const int nbPasMoteur = 400;
const float taille_roue = 7.62;
const int vitesse_max_ent = 100;

//Ecran LCD RGB//
const int colorR = 0;
const int colorG = 0;
const int colorB = 100;

//Capteurs ultrason
/*
Ici on définit la distance max de détection voulue sur les capteurs, laissez par défault si vous ne savez pas.
*/
#define NB_MESURES 3
#define NB_SONAR 2
#define MAX_DISTANCE 100
#define VITESSE_SON 340.0
#define DISTANCE_OB 20

const float delayMesureCapteur =  int(((2*MAX_DISTANCE)/(VITESSE_SON))*1000);
#define DELAI_MESURES_CAPTEURS 1

//Controle moteur
const int nb_pas_360 = 500; //Valeur à trouver de facon empirique
static int currentVitesse = 0;
static int currentDistanceG = 0;
static int currentDistanceD = 0;

//Servo
#define NB_SERVO 1

///COMMANDES///

///VARIABLES GLOBALES///
//int queue[128] = {0}; //LA file d'instruction
//typedef int[2] commande;
QueueArray <int> queue;

unsigned int distance[NB_SONAR];
int CapteurActuel = 0;
float uS;
int i = 0;
bool process_it;
int pos = 0;
int buf[64] = {0};


///INITIALISATION OBJETS
//Le Timer principal
Timer t;
//Ecran LCD//
rgb_lcd lcd;
//Capteurs ultra-sons//
NewPing listeSonar[NB_SONAR] =
{
    NewPing(trigPin1, echoPin1, MAX_DISTANCE),
    NewPing(trigPin2, echoPin2, MAX_DISTANCE)
};
//Moteurs pas à pas//
AccelStepper moteurG(AccelStepper::FULL4WIRE, moteurGIN1, moteurGIN2, moteurGIN3, moteurGIN4, true);
AccelStepper moteurD(AccelStepper::FULL4WIRE, moteurDIN1, moteurDIN2, moteurDIN3, moteurDIN4, true);

MultiStepper moteurs;

//Servomoteurs//
Servo servos[NB_SERVO] =
{
    Servo ()
};

///PROTOTYPES///
void endProg(); //fonction executée à la fin du programme
void funnyaction();
boolean echoCheck(int numCapteur);
float takeValue(int numCapteur);
void detectObstacle();
int avancerPas(AccelStepper parM1, AccelStepper parM2, long parPas, int parVitesseMax);
int avancerTemps(unsigned long parTemps, int parVitesseMax);
void gauche(int parAngle);
void droite(int parAngle);
void arret(AccelStepper parM1, AccelStepper parM2);
void afficherLCD(char msg[]);
int envoyer(char cmd[]);

///INITIALISATION PROGRAMME///
void setup()
{
    Serial.begin (19200); //Pour pouvoir écrire sur le moniteur

    Serial.println("Ping done");

    for (i = 0; i < NB_SERVO; i++)
    {
        servos[i].attach(servoFunny + i);
        //servos[i].write(0);
    }
    Serial.println("Servo done");

    /*
    pinMode(moteurGENA, OUTPUT);
    pinMode(moteurGENB, OUTPUT);
    pinMode(moteurDENA, OUTPUT);
    pinMode(moteurDENB, OUTPUT);
    digitalWrite(moteurGENA, HIGH);
    digitalWrite(moteurGENB, HIGH);
    digitalWrite(moteurDENA, HIGH);
    digitalWrite(moteurDENB, HIGH);*/

    /* Benchmarking des moteurs
    avancerPas(moteurG, moteurD, 1, 150);
    delay(2000);
    avancerPas(moteurG, moteurD, 2, 150);
    delay(2000);
    avancerPas(moteurG, moteurD, 3, 150);
    delay(2000);
    avancerPas(moteurG, moteurD, 4, 150);
    delay(2000);

    avancerPas(moteurG, moteurD, 2, 50);
    delay(2000);
    avancerPas(moteurG, moteurD, 2, 100);
    delay(2000);
    avancerPas(moteurG, moteurD, 2, 150);
    delay(2000);
    avancerPas(moteurG, moteurD, 2, 200);
    delay(2000);
    */

    moteurG.setMaxSpeed(400);// setMaxSpeed 400
    moteurD.setMaxSpeed(400);// setMaxSpeed 400
    moteurG.setSpeed(80);// setSpeed 80
    moteurD.setSpeed(80);//setSpeed 80

    moteurs.addStepper(moteurG);
    moteurs.addStepper(moteurD);
    Serial.println("Stepper done");

    /*
    pinMode(MISO, OUTPUT);
    SPCR |= _BV(SPE);
    pos = 0;
    process_it = false;
    SPI.attachInterrupt();*/
    Serial.println("SPI done");

    lcd.begin(16,2);
    lcd.clear();
    lcd.setRGB(colorR,colorG,colorB);
    lcd.print("-Club Robotique-");
    lcd.setCursor(0, 1);
    lcd.print("----INSA cvl----");
    Serial.println("LCD done");

    //déterminer les codes d'instruction
    queue.push(1);
    queue.push(2);
    queue.push(150);
    queue.push(2);
    queue.push(150);
    queue.push(2);
    queue.push(150);
    queue.push(2);
    queue.push(150);
    queue.push(2);
    queue.push(150);
    Serial.println("Liste de tache done");

    //Initialisation tirette
    pinMode(tirette, INPUT);
    while(digitalRead(tirette)==HIGH)
    {
    }

    //Initialisation timer
    t.after(90000, endProg);
    Serial.println("Tirette et timer done");

    Serial.println("Fin d'initialisation");
}


///BOUCLE PRINCIPALE///
void loop()
{
    t.update();
    //Serial.println("UPDATE");

    long position[2];

    while(!queue.isEmpty())
    {
        t.update(); //Oblige ?
        switch(queue.pop())
        {
            case 1: //LECTURE CAPTEUR
            lcd.print("     _    _     ");
            lcd.setCursor(0, 1);
            lcd.print("     o    o     ");
            for (int i = 0; i < NB_SONAR; i++)
            {
                Serial.print("valeur capteur ");
                Serial.print(i);
                Serial.print(" : ");
                Serial.print(takeValue(i));
                Serial.println(" cm");
            }
            break;

            case 2: //AVANCER D'UN CERTAIN NOMBRE DE PAS
            lcd.print("     ^    ^     ");
            lcd.setCursor(0, 1);
            lcd.print("     o    o     ");
            avancerPas(moteurG,moteurD,queue.pop(), vitesse_max_ent);
            break;

            case 3: //TOURNER A GAUCHE
            break;

            case 4: //TOURNER A DROITE
            break;

            case 5: //AVANCER PENDANT UN CERTAIN TEMPS
            lcd.print("     O    O     ");
            lcd.setCursor(0, 1);
            lcd.print("       []       ");
            avancerTemps(queue.pop(), vitesse_max_ent);
            break;

            case 6: //LECTURE DU MOSI SUR LE REGISTRE SPI
            if (process_it)
            {
                buf [pos] = 0;
                //Serial.println (buf);
                pos = 0;
                process_it = false;
            }
            break;

            default:
            break;
        }
    }
    endProg();

}

///FONCTION DE FIN DE PROGRAMME
//Fonction pour terminer le programme et donc le match
void endProg()
{
    Serial.println("Fin du programme");
    moteurG.disableOutputs();
    moteurD.disableOutputs();
    funnyaction();
    Serial.println("CATAPUUUULTE");
    while(1);
}

//Fonction pour declencher la funnyaction
void funnyaction()
{
    servos[0].write(0); //on active le servo necessaire pour déclenchement funnyaction
    return;
}

///FONCTIONS CAPTEURS ULTRASON
//Permet de récupérer une distance
float takeValue(int numCapteur)
{
    float sum = 0;
    float result;// Attention, si on ne fait pas la conversion directe, les valeurs sont tellement grandes qu'il faut utiliser un long
    int i = 0;
    while (i < NB_MESURES)
    {
        result = listeSonar[numCapteur].ping();

        if (result != 0)
        {
            sum += result / US_ROUNDTRIP_CM;          // On convertis en distance avec la valeur donnée dans la librairie                              // On compte une mesure valide de plus
        }
        //Serial.println(delayMesureCapteur); // Delai entre deux mesures pour éviter les interférences
        i++;
    }
    return sum / NB_MESURES ; // On applique la moyenne
}

//Fonction de callback pour détecter les obstacles --> A REVOIR
void detectObstacle()
{
    //return la valeur du premier capteur qui detecte l'obstacle
    t.update();
    int i;
    //bool hasObstacle = false;
    float distObstacle = 0.0;
    for(i = 0; i < NB_SONAR; i++) //on test tout les capteurs, on peut décider d'esquiver l'obstacle
    {
            /*
            Serial.print("Capteur numero ");
            Serial.println(i);

            //hasObstacle = echoCheck(1);
            if(takeValue(i) < MAX_DISTANCE)
            {
            arret(moteurG,moteurD);
            endProg();
        }
        */
        distObstacle = takeValue(i);
        Serial.print("distObstacle : ");
        Serial.println(distObstacle);
        if(distObstacle < DISTANCE_OB) //si l'on est face a un obstacle
        {
            arret(moteurG,moteurD); //on arrete les moteurs
            while(takeValue(i) < DISTANCE_OB) //on attend qu'il n'y est plus d'obstacle
            {
                t.update();
                delay(100);
            }
            Serial.println("Arret");
        }
        else
        {
            Serial.print("distance : ");
            Serial.println(distObstacle);
            moteurG.run(); //on redemarre les moteurs
            moteurD.run();
        }
    }
    Serial.println("Tick");
}

//Permet de faire un echo check sur un capteur
boolean echoCheck(int numCapteur)
{
    if (listeSonar[numCapteur].check_timer())
    {
        distance[numCapteur] = listeSonar[numCapteur].ping_median(NB_MESURES) / US_ROUNDTRIP_CM;
        return true; //si l'on a quelque chose dans le champ de vision, on préviens
    }
    else
    {
        return false; //pas d'obstacle en vue
    }
}


///FONCTIONS CONTROLE MOTEUR
//fonction pour faire avancer d'un nombre de pas en ligne droite le robot -> A REVOIR
int avancerPas(AccelStepper parM1, AccelStepper parM2, long parPas, int parVitesseMax)
{

    parM1.setCurrentPosition(0);
    parM2.setCurrentPosition(0);
    parM1.moveTo(-parPas);
    parM2.moveTo(parPas);
    while (parM1.distanceToGo() > 0 || parM2.distanceToGo() > 0)
    {
        parM1.setSpeed(parVitesseMax);
        parM2.setSpeed(parVitesseMax);
        parM1.runSpeedToPosition();
        parM2.runSpeedToPosition();
    }
}

//fonction pour faire avancer d'un nombre de pas en ligne droite le robot
int avancerTemps(unsigned long parTemps, int parVitesseMax)
{
    ///A   VERIFIER QUE LE CALLBACK STOP BIEN LA FONCTION
    unsigned long debut = millis();
    Serial.println(parTemps);
    //parM1.setSpeed(parVitesseMax);
    //parM2.setSpeed(parVitesseMax);
    //currentVitesse = parVitesseMax;
    long pos[2] = {100, -100};
    moteurs.moveTo(pos);
    moteurs.run();
    while( debut + parTemps > millis() )
    {
        //Serial.print(millis());
        //Serial.print(" : ");
        Serial.println(millis());
        moteurs.run();
    }

}

//fonction pour tourner à gauche
void gauche(int parAngle)
{
    long pos[2] = {0};
    long var = (360.0/(long)nb_pas_360)*(long)parAngle;
    pos[0] = var;
    pos[1] = -var;
    moteurs.moveTo(pos);
    moteurs.run();
}

//fonction pour tourner à droite
void droite(int parAngle)
{
    gauche(-parAngle);
}

//fonction d'arret des moteurs
void arret(AccelStepper parM1, AccelStepper parM2)
{
    parM1.stop();
    parM2.stop();
}
