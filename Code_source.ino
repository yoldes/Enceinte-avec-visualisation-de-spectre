// ++ Librairies
#include <arduinoFFT.h>
#include <Adafruit_GFX.h>
#include <RGBmatrixPanel.h>   // les 2 dernières librairies nous permettent de gérer l'affichage sur l'écran

// ++ Définitions FFT
#define SAMPLES 64
#define SAMPLING_FREQUENCY 5000  // théorème de Shannon => on pourra analyser jusqu'à 2500 Hz
double vReal[SAMPLES];  // partie réelle du signal analysé
double vImag[SAMPLES];  // partie imaginaire du signal analysé
unsigned long microseconds;   // "chronomètre" du code
unsigned int sampling_period_us; // temps d'échantillonage


// ++ Définitions écran
#define CLK 8
#define LAT A4
#define OE  9
#define A   A1
#define B   A2
#define C   A3        // définition des pins aloués à l'écran
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

// ++ Définitions bouton

const int buttonPin = 10;    // le bouton est connecté au pin numérique n°10
int state = HIGH;             // état actuel du pin du bouton
int previousState = LOW;   // l'état précédent du pin du bouton
unsigned long lastDebounceTime = 0;  // la dernière fois que le bouton a été pressé
unsigned long debounceDelay = 50;
int colors_list[6][3] = {{0, 0, 3}, {3, 0, 0}, {0, 3, 0}, {3, 3, 0}, {3, 0, 3}, {0, 3, 3}};

// ++ Définitions affichage

#define  xres 32
#define  yres 16      // notre matrice fait 32*16 LEDs
char data_avgs[xres];   // moyenne d'amplitude sur 1/32ème de la plage de fréquence étudiée
double vmax;         // amplitude maximale, elle va nous aider à avoir une échelle verticale cohérente
int yvalue;           // hauteur d'une ligne
int r = 0;
int g = 0;
int b = 4;     // à l'allumage, l'écran affiche du bleu
int f = 0;

arduinoFFT FFT = arduinoFFT();

void setup() {

  Serial.begin(115200);
  matrix.begin();                                                      // initialise la matrice
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));    // T=1/f
  delay(50);                                                           // pour attendre que la tension d'entrée se stabilise
  randomSeed(analogRead(5));
}

void loop() {

  vmax = 0;

  // ++ Sampling
  for (int i = 0; i < SAMPLES; i++)
  {
    microseconds = micros();
    vReal[i] = analogRead(0);  // c'est le port A0 qui donne la tension à analyser
    vImag[i] = 0;              // on analyse un signal réel
    while (micros() < (microseconds + sampling_period_us)) {
    }
  }

  // ++ FFT
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // type de fenêtrage du signal
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);                  // calcul de la FFT
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);                    // calcul du module des amplitudes
  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);  // peak est la plus grande amplitude mesurée dans cette itération de loop

  // ++ Affichage moniteur/traceur
  for (int i = 0; i < (SAMPLES / 2); i++)
  {
    Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    Serial.print(" ");
    Serial.println(vReal[i], 1);
  }

  // ++ Réarrangement des résultats de la FFT pour correspondre au nombre de colonnes
  int step = (SAMPLES) / xres;
  int c = 0;
  for (int i = 0; i < (SAMPLES / 2); i += step)
  {
    data_avgs[c] = 0;
    for (int k = 0 ; k < step ; k++) {
      data_avgs[c] = data_avgs[c] + vReal[i + k];
    }
    data_avgs[c] = data_avgs[c] / step;
    c++;
  }

  // ++ Détermination de l'amplitude maximale (on exclut les 2 premières fréquences qui rendent l'échelle illisible)
  for (int i = 2; i < xres + 2; i++) {
    if (vReal[i] > vmax) {
      vmax = vReal[i];
    }
  }

  // ++ Affichage
  matrix.fillScreen(matrix.Color333(0, 0, 0)); // remplit l'écran de noir

  for (int i = 2; i < xres + 2 ; i++) // on commence à la 2ème valeur car les amplitudes des basses fréquences sont trop hautes pour être intéressantes
  {
    yvalue = vReal[i] / vmax * 16 ; // on divise l'amplitude par vmax, ainsi, on est assuré que notre échelle est toujours cohérente, ni trop petite ni trop grande
    matrix.drawLine(i - 2, 15, i - 2, 15 - yvalue, matrix.Color333(r, g, b)); // "i-2" pour recaler les colonnes sur le bord de l'écran
  }                                                                           // on dessine un trait entre les points (i-2,15) et (i-2,15-yvalue)
  displayModeChange();         // Appel de la fonction changement de couleur
}


void displayModeChange() {                 // Fonction permettant de changer la couleur de l'affichage
  for (int j = 0; j < 50 ; j++) {          // On vérifie le niveau du bouton pendant une durée suffisante
    int reading = digitalRead(buttonPin);  
    if (reading == 0)                      // Si on détecte que le bouton n'est pas appuyé, on arrête de lire
    {
      f = 0;
      return (f);
    }
  }
  if (f == 0) {                            // La variable f permet de s'assurer que l'utilisateur a bien relâché le bouton avant de changer la couleur à nouveau
    int a = random(0, 6);                  // On choisit une couleur au hasard
    r = colors_list[a][0];
    g = colors_list[a][1];
    b = colors_list[a][2];
    f = 1;
    return (r, g, b, f);           
  }
}
