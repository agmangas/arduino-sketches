#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Automaton.h>

const int PIN_INPUT_RELAY_ACTIVATION = 11;
const int PIN_OUTPUT_RELAY_COMPLETION = 12;

Atm_button buttonActivation;

bool isActive = false;
bool isRelayOpen = false;

const int NUM_MURCIELAGOS = 4;
const int NUM_LEDS = 2;

int tiraEyes = 10;
int puntosEyes = 8;
int sensibilidad = 10;
int tope = 20;
int limiteChillido = 48;
int agudo = 1100;
int tiempoChillido = 4;

Adafruit_NeoPixel stripBat = Adafruit_NeoPixel(
    puntosEyes,
    tiraEyes,
    NEO_GRB + NEO_KHZ800);

// Pines de murcielagos
int batSensores[NUM_MURCIELAGOS] = {
    A0, A1, A2, A3};

// Chillidos
int batSound[NUM_MURCIELAGOS] = {
    2, 3, 4, 5};

// Puntitos de Ojos murcielago
int batLeds[NUM_MURCIELAGOS][NUM_LEDS] = {
    {0, 1},
    {2, 3},
    {4, 5},
    {6, 7}};

// Contadores de luz
int batLights[NUM_MURCIELAGOS] = {
    0, 0, 0, 0};

// Contadores de victoria
bool solucionNum[NUM_MURCIELAGOS] = {
    false, false, false, false};

// Para omitir comprobaciones
bool batCheck[NUM_MURCIELAGOS] = {
    false, false, false, false};

void lockRelay(uint8_t pin)
{
  digitalWrite(pin, LOW);
}

void openRelay(uint8_t pin)
{
  digitalWrite(pin, HIGH);
}

void initRelay(uint8_t pin)
{
  pinMode(pin, OUTPUT);
  lockRelay(pin);
}

void initLeds()
{
  const int brightness = 10;

  stripBat.begin();
  stripBat.setBrightness(brightness);
  stripBat.show();
  stripBat.clear();
}

void batDark(int murci)
{
  int chillido = random(0, 50);

  if (chillido >= 40)
  {
    stripBat.setPixelColor(batLeds[murci][0], random(100), 0, 0);
    stripBat.setPixelColor(batLeds[murci][1], random(100), 0, 0);
    stripBat.show();
  }
  else
  {
    stripBat.setPixelColor(batLeds[murci][0], 0, 0, 0);
    stripBat.setPixelColor(batLeds[murci][1], 0, 0, 0);
    stripBat.show();
  }
}

void batLight(int murci)
{
  stripBat.setPixelColor(batLeds[murci][0], 200, 200, 0);
  stripBat.setPixelColor(batLeds[murci][1], 200, 200, 0);
  stripBat.show();
}

void batBright(int murci)
{
  stripBat.setPixelColor(batLeds[murci][0], 0, 0, 200);
  stripBat.setPixelColor(batLeds[murci][1], 0, 0, 200);
  stripBat.show();
}

void releOpen()
{
  int qttWin = 0;

  for (int i = 0; i < NUM_MURCIELAGOS; i++)
  {
    if (solucionNum[i] == true)
    {
      qttWin++;
    }
    else
    {
      qttWin = 0;
      break;
    }
  }

  if (qttWin == NUM_MURCIELAGOS)
  {
    Serial.println("Opening relay");
    isRelayOpen = true;
  }
  else
  {
    isRelayOpen = false;
  }
}

void checkBats(int murci)
{
  int batSensor = analogRead(batSensores[murci]);
  int batLevel = map(batSensor, 0, 700, 0, 10);

  int chillido = random(0, 50);

  if (chillido >= limiteChillido)
  {
    tone(batSound[murci], agudo, tiempoChillido);
  }

  if (batLevel >= sensibilidad)
  {
    batLight(murci);
    batCheck[murci] = true;
    batLights[murci] = batLights[murci] + 1;
    tone(batSound[murci], agudo + batLights[murci] * 50, tiempoChillido * 3);
  }
  else
  {
    batCheck[murci] = false;
  }

  if (batLights[murci] >= tope)
  {
    batBright(murci);
    solucionNum[murci] = true;
  }
  else
  {
    batDark(murci);
    solucionNum[murci] = false;
  }
}

void murcielagos()
{
  for (int i = 0; i < NUM_MURCIELAGOS; i++)
  {
    if (!solucionNum[i])
    {
      checkBats(i);
    }
  }
}

void resetOjos()
{
  for (int i = 0; i < NUM_MURCIELAGOS; i++)
  {
    if (!batCheck[i])
    {
      stripBat.setPixelColor(batLeds[i][0], 0, 0, 0);
      stripBat.setPixelColor(batLeds[i][1], 0, 0, 0);
      stripBat.show();
    }
  }
}

void ojitos()
{
  int murcielagoRND = 0;
  int resetRND = 0;

  resetRND = random(14);
  murcielagoRND = random(0, NUM_MURCIELAGOS);

  if (resetRND <= 4 && !batCheck[murcielagoRND])
  {
    resetOjos();
  }

  if (!batCheck[murcielagoRND])
  {
    stripBat.setPixelColor(batLeds[murcielagoRND][0], 200, 0, 0);
    stripBat.setPixelColor(batLeds[murcielagoRND][1], 200, 0, 0);
    stripBat.show();
  }
}

void blinkBats()
{
  const int delayMs = 100;
  const int numLoops = 2;
  const uint32_t color = Adafruit_NeoPixel::Color(250, 250, 250);

  for (int i = 0; i < numLoops; i++)
  {
    stripBat.clear();
    stripBat.show();
    delay(delayMs);
    stripBat.fill(color);
    stripBat.show();
    delay(delayMs);
  }

  stripBat.clear();
  stripBat.show();
}

void activationDebugBlink()
{
  const unsigned long delayMs = 15;
  const unsigned long iters = 40;

  for (uint8_t i = 0; i < iters; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
  }
}

void onActivation(int idx, int v, int up)
{
  Serial.println(F("Detected activation pulse"));
  isActive = true;
  activationDebugBlink();
}

void setup()
{
  Serial.begin(9600);

  initRelay(PIN_OUTPUT_RELAY_COMPLETION);

  const int debounceDelayMs = 2000;

  buttonActivation
      .begin(PIN_INPUT_RELAY_ACTIVATION)
      .debounce(debounceDelayMs)
      .onPress(onActivation);

  initLeds();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println(F("Murcielagos"));
}

void activeLoopDebugBlink()
{
  const unsigned long delayMs = 60;
  const unsigned long iters = 1;

  for (uint8_t i = 0; i < iters; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
  }
}

void inactiveLoopDebugBlink()
{
  const unsigned long longMs = 600;
  const unsigned long shortMs = 50;

  digitalWrite(LED_BUILTIN, HIGH);
  delay(longMs);
  digitalWrite(LED_BUILTIN, LOW);
  delay(shortMs);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  automaton.run();

  /**
   * Descomentar esta línea para saltarse
   * esperar por la activación del relé.
   */
  // isActive = true;

  if (isActive)
  {
    const unsigned long delayLoopMs = 50;

    if (isRelayOpen)
    {
      blinkBats();
      openRelay(PIN_OUTPUT_RELAY_COMPLETION);
    }
    else
    {
      ojitos();
      murcielagos();
      releOpen();
      lockRelay(PIN_OUTPUT_RELAY_COMPLETION);
      activeLoopDebugBlink();
    }

    delay(delayLoopMs);
  }
  else
  {
    inactiveLoopDebugBlink();
  }
}
