/// \file
/// \brief Definitions for board configuration functions
#include "BoardConfig.h"
#include "debugging.h"
#include <cctype>

void printSpecs(BoardSpecs &Specs) {
    printf("Board ID = %s\t", Specs.ID.c_str());
    printf("Network SSID = %s \r\n", Specs.NetworkSSID.c_str());

    printf("Network Password = %s\t", Specs.NetworkPassword.c_str());
    printf("Remote Database Table name = %s\r\n",
           Specs.DatabaseTableName.c_str());

    printf("Remote IP = %s\t", Specs.RemoteIP.c_str());
    printf("Remote Get Request directory = %s\r\n",

           Specs.RemoteDir.c_str());

    printf("remote http port = %d\t", Specs.RemotePort);

    printf("Remote Hostname = %s\r\n", Specs.HostName.c_str());
}
// ============================================================================
BoardSpecs readSDCard(const char *FileName) {

    // try to open sd card
    printf("\r\nReading from SD card...\r\n\n\n");
    FILE *fp = fopen(FileName, "rb");

    BoardSpecs Output;

    if (fp != NULL) {

        // get file size for buffer
        fseek(fp, 0, SEEK_END);
        size_t FileSize = ftell(fp);
        char *Buffer = new char[FileSize + 1];

        // reset file pointer and  read file
        rewind(fp);
        fread(Buffer, sizeof(char), FileSize, fp);
        // set \0 for Cstring compatability
        Buffer[FileSize] = '\0';

        delete[] Buffer; // clean up

        // read config from SD card
        rewind(fp);
        Output = readConfigText(fp);
        printf("\r\n %d Ports were configured\r\n", Output.Ports.size());
        fclose(fp);

    } else {
        printf("\nReading Failed!\r\n");
    }

    return Output;
}

void setBoundsFromID(PortInfo &input, vector<SensorInfo> sensors){
    input.RangeEnd = sensors[input.SensorID].RangeEnd;
    input.RangeEnd = sensors[input.SensorID].RangeEnd;
}

// ============================================================================
BoardSpecs readConfigText(FILE *fp) {
    BoardSpecs Specs;
    Specs.Ports.reserve(10);   // reserve space for ports
    Specs.Sensors.reserve(10); // and sensor types

    int prtCnt = 0; // # of ports

    // temporary buffer
    char Buffer[BUFFLEN];

    // read through and get sensor ids before getting the port information
    while (fgets(Buffer, BUFFLEN, fp) != NULL) {

        // if the line has 'SensorID' in it, then get the sensor info from it
        if (strstr(Buffer, "SensorID") && Buffer[0] == 'S') {
            SensorInfo tmp;

            // get the id number
            strtok(Buffer, ":"); // get past the :

            const char token[] = ","; // token to split values

            char *value = strtok(NULL, token);
            tmp.Type = value;

            // get the unit of the sensor
            value = strtok(NULL, token);
            tmp.Unit = value;

            // get unit multiplier
            value = strtok(NULL, token);
            tmp.Multiplier = atof(value);

            // get range start
            value = strtok(NULL, token);
            tmp.RangeStart = atof(value);

            // get range end till end of line
            value = strtok(NULL, "\n");
            tmp.RangeEnd = atof(value);

            Specs.Sensors.push_back(tmp); // store those values
            printf("Sensor type: %s, Unit: %s, range start: %f, range-end: %f\r\n", tmp.Type.c_str(),
                   tmp.Unit.c_str(), tmp.RangeStart, tmp.RangeEnd);
        }
    }

    rewind(fp); // get ready to read again.

    while (fgets(Buffer, BUFFLEN, fp) != NULL) {

        const char *s = ":";

        // save the remote connection info
        if (Buffer[0] == 'C' && strstr(Buffer, "ConnInfo")) {

            // we don't need the first token
            char *tmp = strtok(Buffer, s);

            Specs.RemoteIP = strtok(NULL, s);

            // make sure there is a digit to convert, and set an error value
            tmp = strtok(NULL, s);
            if (isdigit(tmp[0])) {
                Specs.RemotePort = atoi(tmp);
            } else {
                Specs.RemotePort = 0;
            }

            Specs.HostName = strtok(NULL, s);

            Specs.RemoteDir = strtok(NULL, "\n");
        }

        // checks the character at the beginning of each line
        if (Buffer[0] == 'B' && strstr(Buffer, "Board")) {

            // get board id and assign it
            Specs.ID = strtok(Buffer, s);

            // get WIFI SSID and assign it
            Specs.NetworkSSID = strtok(NULL, s);

            // get WIFI Password and assign it
            Specs.NetworkPassword = strtok(NULL, s);

            // getting and assigning Database tablename
            Specs.DatabaseTableName = strtok(NULL, s); // opt opportunity
        } else if (Buffer[0] == 'P') { // if a port description is detected

            // hold a Port entry
            // then assign them to the vector in Specs
            PortInfo tmp;

            ++prtCnt;

            
            // grab the port name too
            tmp.Name = strtok(Buffer, ":");

            tmp.SensorID = atoi(strtok(NULL, "\n"));

            // remove whitespace from the name
            size_t SpaceDex = tmp.Name.find_first_of(' ');
            while (SpaceDex != string::npos) {
                tmp.Name.erase(SpaceDex);
                SpaceDex = tmp.Name.find_first_of(' ');
            }

            // get port multiplier
            tmp.Multiplier = setUnitMultiplier(Specs.Sensors, tmp.SensorID);

            // get sensorname
            tmp.Description = getSensorName(Specs.Sensors, tmp.SensorID);

            setBoundsFromID(tmp, Specs.Sensors);

            printf("Port Info: name= %s id=  %d Multiplier= %0.2f description=%s\r\n", tmp.Name.c_str(),
                   tmp.SensorID, tmp.Multiplier, tmp.Description.c_str());

            // store the port in the boardSpecs struct only if it means anything
            if (tmp.Multiplier != 0.0f) {
                Specs.Ports.push_back(tmp);
            }
        }
    }
    printSpecs(Specs);
    return Specs;
}

// ============================================================================
float setUnitMultiplier(vector<SensorInfo> &Sensors, size_t Sens_ID) {
    // if there is no sensor for that id, then return 0
    if (Sens_ID >= Sensors.size()) {
        return 0.0;
    }

    return Sensors[Sens_ID].Multiplier;
}

// ============================================================================
string getSensorName(vector<SensorInfo> &Sensors, size_t Sens_ID) {

    // if there is no sensor for that id, then return error message
    if (Sens_ID >= Sensors.size()) {
        return "No Sensor";
    }

    const char *in = " in ";

    size_t str_size = strlen(in) + Sensors[Sens_ID].Type.size() +
                      Sensors[Sens_ID].Unit.size();

    string ret_str;
    ret_str.reserve(str_size);
    ret_str.append(Sensors[Sens_ID].Type);
    ret_str.append(in);
    ret_str.append(Sensors[Sens_ID].Unit);

    return ret_str;
}
