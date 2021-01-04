// Universum | Universum Projects > CityGuide

// Andrei Florian 21/FEB/2019

#define ARDUINO_MKR

#include <SDHCI.h>
#include <GNSS.h>
#include <Audio.h>
#include <Universum_Logo.h>

#define STRING_BUFFER_SIZE 128

SDClass SD;
File myFile;
File soundFile;
static SpGnss gnss;
AudioClass *theAudio;

struct Landmark
{
  // strings
  String name;
  String sound;
  String rawLocation;

  // actual location
  float latitude;
  float longitude;

  // grid co-ordinates
  float maxLat;
  float minLat;
  float maxLng;
  float minLng;

  // extras
  bool visited;
};

Landmark landmark[10];

float gpsLatitude;
float gpsLongitude;

bool proDebug = false; // debugging?
int newLine = 0;

void terminateLEDS()
{
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
}

void startGPS(bool hot)
{
  Serial.println("");
  digitalWrite(LED0, HIGH);

  Serial.println("Initialising GNSS");
  Serial.println("  OK - Setting Debug");
  gnss.setDebugMode(PrintInfo); // set the mode to print info

  Serial.println("Initialising Module");
  if(gnss.begin() != 0)
  {
    Serial.println("  Error - Module Failed to Initialise");
    digitalWrite(LED0, LOW);
    while(1) {};
  }
  else
  {
    Serial.println("  OK - Setting Elements");
    gnss.select(QZ_L1CA);
    gnss.select(QZ_L1S);

    Serial.println("  OK - Starting Positioning");
    if(hot)
    {
      if(gnss.start(HOT_START) != 0)
      {
        Serial.println("  Error - Start Failed");
        digitalWrite(LED0, LOW);
        while(1) {};
      }
      else
      {
        Serial.println("  Success - GNSS Setup Complete");
      }
    }
    else
    {
      if(gnss.start(COLD_START) != 0)
      {
        Serial.println("  Error - Start Failed");
        digitalWrite(LED0, LOW);
        while(1) {};
      }
      else
      {
        Serial.println("  Success - GNSS Setup Complete");
      }
    }
  }
  
  digitalWrite(LED0, HIGH);
  delay(500);
}

void processGPS(SpNavData *pNavData)
{
  char dataBuffer[STRING_BUFFER_SIZE];

  // print number of satellites
  snprintf(dataBuffer, STRING_BUFFER_SIZE, "numSat:%2d, ", pNavData->numSatellites);
  Serial.print(dataBuffer);

  // print the location data
  Serial.print(" ");
  if(pNavData->posFixMode == FixInvalid)
  {
    Serial.print("NO FIX  ");
  }
  else
  {
    Serial.print("FIX     ");
  }

  if(pNavData->posDataExist == 0)
  {
    Serial.println("No Geolocation");
  }
  else
  {
    gpsLatitude = pNavData->latitude;
    gpsLongitude = pNavData->longitude;
    Serial.print(gpsLatitude, 6); Serial.print(","); Serial.println(gpsLongitude, 6);
    checkLocation();
  }
}

void checkLocation()
{
  for(int i = 0; i < (newLine + 1); i++)
  {
    if(gpsLatitude <= landmark[i].maxLat && gpsLatitude >= landmark[i].minLat &&
       gpsLongitude <= landmark[i].maxLng && gpsLongitude >= landmark[i].minLng )
    {
      if(landmark[i].visited)
      {
        Serial.println("  [1/2] Already Visited");
        delay(500);
      }
      else
      {
        Serial.println("  [1/2] First Visit");
        delay(1000);

        Serial.println("  OK - Checking Data");
        Serial.print("  landmark name       "); Serial.println(landmark[i].name);
        Serial.print("  landmark sound file "); Serial.println(landmark[i].sound);

        Serial.println("  [2/2] Calling 'playFile' with audio file");
        Serial.println("  OK - Calling Function");
        Serial.println("");
        attachFile(landmark[i].sound);
        landmark[i].visited = true;
        break;
      }
    }
  }
}

void attachFile(String fileName)
{
  Serial.println("");

  Serial.println("Setting Up File");
  Serial.println("________________________________________");
  Serial.println("Finalising Setup");
  digitalWrite(LED2, HIGH);
  delay(500);
  digitalWrite(LED2, LOW);

  Serial.println("");
  Serial.println("Locating File");

  Serial.println("  OK - Opening File");
  Serial.print("  OK - Locating "); Serial.println(fileName);
  soundFile = SD.open(fileName);

  if(!soundFile)
  {
    Serial.println("  Fatal Error - File Not Present");
    Serial.println("________________________________________");
    Serial.println("");
    delay(500);
    return;
  }
  else
  {
    Serial.println("  Success - File Located");
  }

  Serial.println("");
  Serial.println("Analysing Format");
  Serial.println("  OK - Getting Frames to Analyse");
  int err = theAudio->writeFrames(AudioClass::Player0, soundFile);

  Serial.println("  OK - Analysing Data");
  if(err != AUDIOLIB_ECODE_OK)
  {
    Serial.println("  Fatal Error - File Formatation is Bad");
    soundFile.close();
    Serial.println("________________________________________");
  }
  else
  {
    Serial.println("  Success - Formatation is Good");
  }

  Serial.println("________________________________________");
  Serial.println("");
  playFile(fileName);
}

void playFile(String fileName)
{
  Serial.println("Playing File");
  Serial.println("________________________________________");
  Serial.println("  OK - Pausing GNSS");
  gnss.stop();
  
  Serial.print("Playing "); Serial.println(fileName);
  theAudio->startPlayer(AudioClass::Player0);
  digitalWrite(LED2, HIGH);
  
  while(1) // main playback loop
  {
    Serial.print(".");

    int rawPot = analogRead(A0);
    int volume = map(rawPot, 0, 1024, -700, 0);
    theAudio->setVolume(volume); // set the volume to the position of the potentiometer
    
    int err = theAudio->writeFrames(AudioClass::Player0, soundFile);

    if(err == AUDIOLIB_ECODE_FILEEND) // end of file record
    {
      Serial.println("");
      break;
    }

    if(err)
    {
      Serial.println("");
      Serial.println("  Error - Playback Error");
      Serial.print("  OK - Error ID "); Serial.println(err);
      break;
    }

    usleep(40000);
  }

  theAudio->stopPlayer(AudioClass::Player0);
  soundFile.close();
  digitalWrite(LED2, LOW);
  Serial.println("  OK - Playback Terminated");
  delay(500);
  Serial.println("  OK - Restarting GNSS");
  startGPS(true);
  Serial.println("________________________________________");
  Serial.println("");
  delay(500);
}

void getData()
{
  Serial.println("Getting Data");
  Serial.println("________________________________________");
  Serial.println("Locating File");

  Serial.println("  OK - Preparing Variables");
  int space = 0;
  int comma = 0;

  String localName[10];
  String localSound[10];
  String localRawLocation[10];
  String localLatitude[10];
  String localLongitude[10];

  Serial.println("  OK - Opening File 'datalog.txt'");
  myFile = SD.open("datalog.txt");
  
  Serial.println("  OK - Verifying Presence");
  if(myFile)
  {
    Serial.println("  Success - File Loaded");
    Serial.println("");

    Serial.println("Extracting Data");
    Serial.println("  OK - Beginning Final Extraction");
  
    while(myFile.available())
    {
      char c = myFile.read();
      
      if(c == '\n')
      {
        newLine++;
      }
      else if(c == ' ')
      {
        space++;
      }
      else
      {
        if(space == 0)
        {
          localName[newLine] += c;
        }
        else if(space == 1)
        {
          if(c == ',')
          {
            comma++;
          }
          else if(comma == 0)
          {
            localLatitude[newLine] += c;
          }
          else
          {
            localLongitude[newLine] += c;
          }

          localRawLocation[newLine] += c;
        }
        else if(space == 2)
        {
          localSound[newLine] += c;

          if(c == '3')
          {
            space = 0;
            comma = 0;
          }
        }
      }
    }

    // parse the local data into the struct
    for(int i = 0; i < (newLine + 1); i++)
    {
      landmark[i].name = localName[i];
      landmark[i].sound = localSound[i];
      landmark[i].rawLocation = localRawLocation[i];
      landmark[i].latitude = localLatitude[i].toFloat();
      landmark[i].longitude = localLongitude[i].toFloat();
    }
  }
  else
  {
    Serial.println("  Error - File not Present");
    Serial.println("  OK - Terminating Algorithm");
    Serial.println("________________________________________");
    Serial.println("");
    terminateLEDS();
    while(1) {};
  }

  myFile.close();
  Serial.println("  Success - Data Loaded Locally");
  Serial.println("");
  feedback();
  drawGrid();
}

void feedback()
{
  Serial.println("Data Feedback");
  Serial.println("  OK - Dumping All Data");
  Serial.println("");
  
  for(int i = 0; i < (newLine + 1); i++)
  {
    Serial.print("Struct "); Serial.println(i);
    Serial.print("  location name   "); Serial.println(landmark[i].name);
    Serial.print("  sound file      "); Serial.println(landmark[i].sound);
    Serial.print("  raw location    "); Serial.println(landmark[i].rawLocation);
    Serial.print("  latitude        "); Serial.println(landmark[i].latitude, 4);
    Serial.print("  longitude       "); Serial.println(landmark[i].longitude, 4);
  }

  Serial.println("");
  Serial.println("  Success - Data Dumped");
  Serial.println("________________________________________");
  Serial.println("");
}

void drawGrid()
{
  Serial.println("Generating Grids");
  Serial.println("________________________________________");
  Serial.println("Mapping Grid Around Co-ordinates");
  Serial.println("  OK - Looping through landmarks");

  for(int i = 0; i < (newLine + 1); i++)
  {
    landmark[i].maxLat = landmark[i].latitude + 0.001;
    landmark[i].minLat = landmark[i].latitude - 0.001;
    landmark[i].maxLng = landmark[i].longitude + 0.001;
    landmark[i].minLng = landmark[i].longitude - 0.001;
  }

  Serial.println("  Success - Grids Generated");
  Serial.println("");
  
  Serial.println("Dumping Grid Data");
  Serial.println("  OK - Dumping Grid Data");
  Serial.println("");
  
  for(int i = 0; i < (newLine + 1); i++)
  {
    Serial.print("Struct "); Serial.println(i);
    Serial.print("  max lat   "); Serial.println(landmark[i].maxLat, 4);
    Serial.print("  min lat   "); Serial.println(landmark[i].minLat, 4);
    Serial.print("  max lng   "); Serial.println(landmark[i].maxLng, 4);
    Serial.print("  min lng   "); Serial.println(landmark[i].minLng, 4);
  }

  Serial.println("");
  Serial.println("  Success - Data Dump Complete");
  Serial.println("________________________________________");
  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  digitalWrite(LED0, HIGH);
  
  if(proDebug)
  {
    while(!Serial) {};
    Serial.println("CityGuide");
    Serial.println("  by Andrei Florian");
    Serial.println("");
    Serial.println("");
  }

  delay(500);

  digitalWrite(LED0, HIGH);
  Serial.println("Initialising Audio");
  Serial.println("  OK - Starting Initialisation");
  theAudio = AudioClass::getInstance();
  theAudio->begin();

  Serial.println("  OK - Setting Clock");
  theAudio->setRenderingClockMode(AS_CLKMODE_NORMAL);
  digitalWrite(LED0, LOW);

  Serial.println("  OK - Setting Device to Speaker/Headphones");
  theAudio->setPlayerMode(AS_SETPLAYER_OUTPUTDEVICE_SPHP);

  Serial.println("  OK - Getting MP3 Decoder");
  // if not installed upload - Reference / Sony Spresense / MP3DecoderInstaller
  err_t err = theAudio->initPlayer(AudioClass::Player0, AS_CODECTYPE_MP3, "/mnt/spif/BIN", AS_SAMPLINGRATE_AUTO, AS_CHANNEL_STEREO);
  
  if(err != AUDIOLIB_ECODE_OK)
  {
    Serial.println("  Fatal Error - Initialisation Failed");
    terminateLEDS();
    while(1) {};
  }
  else
  {
    Serial.println("  Success - Initialisation Complete");
  }

  delay(500);
  startGPS(false);
  Serial.println("");
  Serial.println("");
  getData();
}

void loop()
{
  if(gnss.waitUpdate(-1))
  {
    // get navigation data
    SpNavData NavData;
    gnss.getNavData(&NavData);

    if(NavData.posDataExist && (NavData.posFixMode != FixInvalid)) // if fixed on satelite
    {
      digitalWrite(LED1, HIGH);
    }
    else
    {
      digitalWrite(LED1, HIGH);
      delay(500);
      digitalWrite(LED1, LOW);
      delay(500);
    }

    processGPS(&NavData);
  }
}
