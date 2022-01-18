#include "esphome.h"

#define MAX_ZONE 32
#define MAX_AREE 8

class Bentel_Kyo32 : public PollingComponent, public UARTDevice, public CustomAPIDevice
{
public:
	Bentel_Kyo32(UARTComponent *parent) : UARTDevice(parent) {}

	BinarySensor* zona = new BinarySensor[MAX_ZONE];
	BinarySensor *zona_1 = &zona[0];
	BinarySensor *zona_2 = &zona[1];
	BinarySensor *zona_3 = &zona[2];
	BinarySensor *zona_4 = &zona[3];
	BinarySensor *zona_5 = &zona[4];
	BinarySensor *zona_6 = &zona[5];
	BinarySensor *zona_7 = &zona[6];
	BinarySensor *zona_8 = &zona[7];
	BinarySensor *zona_9 = &zona[8];
	BinarySensor *zona_10 = &zona[9];
	BinarySensor *zona_11 = &zona[10];
	BinarySensor *zona_12 = &zona[11];
	BinarySensor *zona_13 = &zona[12];
	BinarySensor *zona_14 = &zona[13];
	BinarySensor *zona_15 = &zona[14];
	BinarySensor *zona_16 = &zona[15];
	BinarySensor *zona_17 = &zona[16];
	BinarySensor *zona_18 = &zona[17];
	BinarySensor *zona_19 = &zona[18];
	BinarySensor *zona_20 = &zona[19];
	BinarySensor *zona_21 = &zona[20];
	BinarySensor *zona_22 = &zona[21];
	BinarySensor *zona_23 = &zona[22];
	BinarySensor *zona_24 = &zona[23];
	BinarySensor *zona_25 = &zona[24];
	BinarySensor *zona_26 = &zona[25];
	BinarySensor *zona_27 = &zona[26];
	BinarySensor *zona_28 = &zona[27];
	BinarySensor *zona_29 = &zona[28];
	BinarySensor *zona_30 = &zona[29];
	BinarySensor *zona_31 = &zona[30];
	BinarySensor *zona_32 = &zona[31];

	BinarySensor* zona_sabotaggio = new BinarySensor[MAX_ZONE];
	BinarySensor *zona_sabotaggio_1 = &zona_sabotaggio[0];
	BinarySensor *zona_sabotaggio_2 = &zona_sabotaggio[1];
	BinarySensor *zona_sabotaggio_3 = &zona_sabotaggio[2];
	BinarySensor *zona_sabotaggio_4 = &zona_sabotaggio[3];
	BinarySensor *zona_sabotaggio_5 = &zona_sabotaggio[4];
	BinarySensor *zona_sabotaggio_6 = &zona_sabotaggio[5];
	BinarySensor *zona_sabotaggio_7 = &zona_sabotaggio[6];
	BinarySensor *zona_sabotaggio_8 = &zona_sabotaggio[7];
	BinarySensor *zona_sabotaggio_9 = &zona_sabotaggio[8];
	BinarySensor *zona_sabotaggio_10 = &zona_sabotaggio[9];
	BinarySensor *zona_sabotaggio_11 = &zona_sabotaggio[10];
	BinarySensor *zona_sabotaggio_12 = &zona_sabotaggio[11];
	BinarySensor *zona_sabotaggio_13 = &zona_sabotaggio[12];
	BinarySensor *zona_sabotaggio_14 = &zona_sabotaggio[13];
	BinarySensor *zona_sabotaggio_15 = &zona_sabotaggio[14];
	BinarySensor *zona_sabotaggio_16 = &zona_sabotaggio[15];
	BinarySensor *zona_sabotaggio_17 = &zona_sabotaggio[16];
	BinarySensor *zona_sabotaggio_18 = &zona_sabotaggio[17];
	BinarySensor *zona_sabotaggio_19 = &zona_sabotaggio[18];
	BinarySensor *zona_sabotaggio_20 = &zona_sabotaggio[19];
	BinarySensor *zona_sabotaggio_21 = &zona_sabotaggio[20];
	BinarySensor *zona_sabotaggio_22 = &zona_sabotaggio[21];
	BinarySensor *zona_sabotaggio_23 = &zona_sabotaggio[22];
	BinarySensor *zona_sabotaggio_24 = &zona_sabotaggio[23];
	BinarySensor *zona_sabotaggio_25 = &zona_sabotaggio[24];
	BinarySensor *zona_sabotaggio_26 = &zona_sabotaggio[25];
	BinarySensor *zona_sabotaggio_27 = &zona_sabotaggio[26];
	BinarySensor *zona_sabotaggio_28 = &zona_sabotaggio[27];
	BinarySensor *zona_sabotaggio_29 = &zona_sabotaggio[28];
	BinarySensor *zona_sabotaggio_30 = &zona_sabotaggio[29];
	BinarySensor *zona_sabotaggio_31 = &zona_sabotaggio[30];
	BinarySensor *zona_sabotaggio_32 = &zona_sabotaggio[31];
	
	BinarySensor* zona_esclusa = new BinarySensor[MAX_ZONE];
	BinarySensor *zona_esclusa_1 = &zona_esclusa[0];
	BinarySensor *zona_esclusa_2 = &zona_esclusa[1];
	BinarySensor *zona_esclusa_3 = &zona_esclusa[2];
	BinarySensor *zona_esclusa_4 = &zona_esclusa[3];
	BinarySensor *zona_esclusa_5 = &zona_esclusa[4];
	BinarySensor *zona_esclusa_6 = &zona_esclusa[5];
	BinarySensor *zona_esclusa_7 = &zona_esclusa[6];
	BinarySensor *zona_esclusa_8 = &zona_esclusa[7];
	BinarySensor *zona_esclusa_9 = &zona_esclusa[8];
	BinarySensor *zona_esclusa_10= &zona_esclusa[9];
	BinarySensor *zona_esclusa_11= &zona_esclusa[10];
	BinarySensor *zona_esclusa_12= &zona_esclusa[11];
	BinarySensor *zona_esclusa_13= &zona_esclusa[12];
	BinarySensor *zona_esclusa_14= &zona_esclusa[13];
	BinarySensor *zona_esclusa_15= &zona_esclusa[14];
	BinarySensor *zona_esclusa_16= &zona_esclusa[15];
	BinarySensor *zona_esclusa_17= &zona_esclusa[16];
	BinarySensor *zona_esclusa_18= &zona_esclusa[17];
	BinarySensor *zona_esclusa_19= &zona_esclusa[18];
	BinarySensor *zona_esclusa_20= &zona_esclusa[19];
	BinarySensor *zona_esclusa_21= &zona_esclusa[20];
	BinarySensor *zona_esclusa_22= &zona_esclusa[21];
	BinarySensor *zona_esclusa_23= &zona_esclusa[22];
	BinarySensor *zona_esclusa_24= &zona_esclusa[23];
	BinarySensor *zona_esclusa_25= &zona_esclusa[24];
	BinarySensor *zona_esclusa_26= &zona_esclusa[25];
	BinarySensor *zona_esclusa_27= &zona_esclusa[26];
	BinarySensor *zona_esclusa_28= &zona_esclusa[27];
	BinarySensor *zona_esclusa_29= &zona_esclusa[28];
	BinarySensor *zona_esclusa_30= &zona_esclusa[29];
	BinarySensor *zona_esclusa_31= &zona_esclusa[30];
	BinarySensor *zona_esclusa_32= &zona_esclusa[31];
	
	BinarySensor* memoria_allarme_zona = new BinarySensor[MAX_ZONE];
	BinarySensor *memoria_allarme_zona_1 = &memoria_allarme_zona[0];
	BinarySensor *memoria_allarme_zona_2 = &memoria_allarme_zona[1];
	BinarySensor *memoria_allarme_zona_3 = &memoria_allarme_zona[2];
	BinarySensor *memoria_allarme_zona_4 = &memoria_allarme_zona[3];
	BinarySensor *memoria_allarme_zona_5 = &memoria_allarme_zona[4];
	BinarySensor *memoria_allarme_zona_6 = &memoria_allarme_zona[5];
	BinarySensor *memoria_allarme_zona_7 = &memoria_allarme_zona[6];
	BinarySensor *memoria_allarme_zona_8 = &memoria_allarme_zona[7];
	BinarySensor *memoria_allarme_zona_9 = &memoria_allarme_zona[8];
	BinarySensor *memoria_allarme_zona_10 = &memoria_allarme_zona[9];
	BinarySensor *memoria_allarme_zona_11 = &memoria_allarme_zona[10];
	BinarySensor *memoria_allarme_zona_12 = &memoria_allarme_zona[11];
	BinarySensor *memoria_allarme_zona_13 = &memoria_allarme_zona[12];
	BinarySensor *memoria_allarme_zona_14 = &memoria_allarme_zona[13];
	BinarySensor *memoria_allarme_zona_15 = &memoria_allarme_zona[14];
	BinarySensor *memoria_allarme_zona_16 = &memoria_allarme_zona[15];
	BinarySensor *memoria_allarme_zona_17 = &memoria_allarme_zona[16];
	BinarySensor *memoria_allarme_zona_18 = &memoria_allarme_zona[17];
	BinarySensor *memoria_allarme_zona_19 = &memoria_allarme_zona[18];
	BinarySensor *memoria_allarme_zona_20 = &memoria_allarme_zona[19];
	BinarySensor *memoria_allarme_zona_21 = &memoria_allarme_zona[20];
	BinarySensor *memoria_allarme_zona_22 = &memoria_allarme_zona[21];
	BinarySensor *memoria_allarme_zona_23 = &memoria_allarme_zona[22];
	BinarySensor *memoria_allarme_zona_24 = &memoria_allarme_zona[23];
	BinarySensor *memoria_allarme_zona_25 = &memoria_allarme_zona[24];
	BinarySensor *memoria_allarme_zona_26 = &memoria_allarme_zona[25];
	BinarySensor *memoria_allarme_zona_27 = &memoria_allarme_zona[26];
	BinarySensor *memoria_allarme_zona_28 = &memoria_allarme_zona[27];
	BinarySensor *memoria_allarme_zona_29 = &memoria_allarme_zona[28];
	BinarySensor *memoria_allarme_zona_30 = &memoria_allarme_zona[29];
	BinarySensor *memoria_allarme_zona_31 = &memoria_allarme_zona[30];
	BinarySensor *memoria_allarme_zona_32 = &memoria_allarme_zona[31];
	
	BinarySensor* memoria_sabotaggio_zona = new BinarySensor[MAX_ZONE];
	BinarySensor *memoria_sabotaggio_zona_1 = &memoria_sabotaggio_zona[0];
	BinarySensor *memoria_sabotaggio_zona_2 = &memoria_sabotaggio_zona[1];
	BinarySensor *memoria_sabotaggio_zona_3 = &memoria_sabotaggio_zona[2];
	BinarySensor *memoria_sabotaggio_zona_4 = &memoria_sabotaggio_zona[3];
	BinarySensor *memoria_sabotaggio_zona_5 = &memoria_sabotaggio_zona[4];
	BinarySensor *memoria_sabotaggio_zona_6 = &memoria_sabotaggio_zona[5];
	BinarySensor *memoria_sabotaggio_zona_7 = &memoria_sabotaggio_zona[6];
	BinarySensor *memoria_sabotaggio_zona_8 = &memoria_sabotaggio_zona[7];
	BinarySensor *memoria_sabotaggio_zona_9 = &memoria_sabotaggio_zona[8];
	BinarySensor *memoria_sabotaggio_zona_10 = &memoria_sabotaggio_zona[9];
	BinarySensor *memoria_sabotaggio_zona_11 = &memoria_sabotaggio_zona[10];
	BinarySensor *memoria_sabotaggio_zona_12 = &memoria_sabotaggio_zona[11];
	BinarySensor *memoria_sabotaggio_zona_13 = &memoria_sabotaggio_zona[12];
	BinarySensor *memoria_sabotaggio_zona_14 = &memoria_sabotaggio_zona[13];
	BinarySensor *memoria_sabotaggio_zona_15 = &memoria_sabotaggio_zona[14];
	BinarySensor *memoria_sabotaggio_zona_16 = &memoria_sabotaggio_zona[15];
	BinarySensor *memoria_sabotaggio_zona_17 = &memoria_sabotaggio_zona[16];
	BinarySensor *memoria_sabotaggio_zona_18 = &memoria_sabotaggio_zona[17];
	BinarySensor *memoria_sabotaggio_zona_19 = &memoria_sabotaggio_zona[18];
	BinarySensor *memoria_sabotaggio_zona_20 = &memoria_sabotaggio_zona[19];
	BinarySensor *memoria_sabotaggio_zona_21 = &memoria_sabotaggio_zona[20];
	BinarySensor *memoria_sabotaggio_zona_22 = &memoria_sabotaggio_zona[21];
	BinarySensor *memoria_sabotaggio_zona_23 = &memoria_sabotaggio_zona[22];
	BinarySensor *memoria_sabotaggio_zona_24 = &memoria_sabotaggio_zona[23];
	BinarySensor *memoria_sabotaggio_zona_25 = &memoria_sabotaggio_zona[24];
	BinarySensor *memoria_sabotaggio_zona_26 = &memoria_sabotaggio_zona[25];
	BinarySensor *memoria_sabotaggio_zona_27 = &memoria_sabotaggio_zona[26];
	BinarySensor *memoria_sabotaggio_zona_28 = &memoria_sabotaggio_zona[27];
	BinarySensor *memoria_sabotaggio_zona_29 = &memoria_sabotaggio_zona[28];
	BinarySensor *memoria_sabotaggio_zona_30 = &memoria_sabotaggio_zona[29];
	BinarySensor *memoria_sabotaggio_zona_31 = &memoria_sabotaggio_zona[30];
	BinarySensor *memoria_sabotaggio_zona_32 = &memoria_sabotaggio_zona[31];

	BinarySensor* allarme_area = new BinarySensor[MAX_AREE];
	BinarySensor *allarme_area_1 = &allarme_area[0];
	BinarySensor *allarme_area_2 = &allarme_area[1];
	BinarySensor *allarme_area_3 = &allarme_area[2];
	BinarySensor *allarme_area_4 = &allarme_area[3];
	BinarySensor *allarme_area_5 = &allarme_area[4];
	BinarySensor *allarme_area_6 = &allarme_area[5];
	BinarySensor *allarme_area_7 = &allarme_area[6];
	BinarySensor *allarme_area_8 = &allarme_area[7];

	BinarySensor* inserimento_totale_area = new BinarySensor[MAX_AREE];
	BinarySensor *inserimento_totale_area_1 = &inserimento_totale_area[0];
	BinarySensor *inserimento_totale_area_2 = &inserimento_totale_area[1];
	BinarySensor *inserimento_totale_area_3 = &inserimento_totale_area[2];
	BinarySensor *inserimento_totale_area_4 = &inserimento_totale_area[3];
	BinarySensor *inserimento_totale_area_5 = &inserimento_totale_area[4];
	BinarySensor *inserimento_totale_area_6 = &inserimento_totale_area[5];
	BinarySensor *inserimento_totale_area_7 = &inserimento_totale_area[6];
	BinarySensor *inserimento_totale_area_8 = &inserimento_totale_area[7];

	BinarySensor* inserimento_parziale_area = new BinarySensor[MAX_AREE];
	BinarySensor *inserimento_parziale_area_1 = &inserimento_parziale_area[0];
	BinarySensor *inserimento_parziale_area_2 = &inserimento_parziale_area[1];
	BinarySensor *inserimento_parziale_area_3 = &inserimento_parziale_area[2];
	BinarySensor *inserimento_parziale_area_4 = &inserimento_parziale_area[3];
	BinarySensor *inserimento_parziale_area_5 = &inserimento_parziale_area[4];
	BinarySensor *inserimento_parziale_area_6 = &inserimento_parziale_area[5];
	BinarySensor *inserimento_parziale_area_7 = &inserimento_parziale_area[6];
	BinarySensor *inserimento_parziale_area_8 = &inserimento_parziale_area[7];

	BinarySensor* inserimento_parziale_ritardo_0_area = new BinarySensor[MAX_AREE];
	BinarySensor *inserimento_parziale_ritardo_0_area_1 = &inserimento_parziale_ritardo_0_area[0];
	BinarySensor *inserimento_parziale_ritardo_0_area_2 = &inserimento_parziale_ritardo_0_area[1];
	BinarySensor *inserimento_parziale_ritardo_0_area_3 = &inserimento_parziale_ritardo_0_area[2];
	BinarySensor *inserimento_parziale_ritardo_0_area_4 = &inserimento_parziale_ritardo_0_area[3];
	BinarySensor *inserimento_parziale_ritardo_0_area_5 = &inserimento_parziale_ritardo_0_area[4];
	BinarySensor *inserimento_parziale_ritardo_0_area_6 = &inserimento_parziale_ritardo_0_area[5];
	BinarySensor *inserimento_parziale_ritardo_0_area_7 = &inserimento_parziale_ritardo_0_area[6];
	BinarySensor *inserimento_parziale_ritardo_0_area_8 = &inserimento_parziale_ritardo_0_area[7];

	BinarySensor* disinserita_area = new BinarySensor[MAX_AREE];
	BinarySensor *disinserita_area_1 = &disinserita_area[0];
	BinarySensor *disinserita_area_2 = &disinserita_area[1];
	BinarySensor *disinserita_area_3 = &disinserita_area[2];
	BinarySensor *disinserita_area_4 = &disinserita_area[3];
	BinarySensor *disinserita_area_5 = &disinserita_area[4];
	BinarySensor *disinserita_area_6 = &disinserita_area[5];
	BinarySensor *disinserita_area_7 = &disinserita_area[6];
	BinarySensor *disinserita_area_8 = &disinserita_area[7];

	BinarySensor* stato_uscita = new BinarySensor[MAX_AREE];
	BinarySensor *stato_uscita_1 = &stato_uscita[0];
	BinarySensor *stato_uscita_2 = &stato_uscita[1];
	BinarySensor *stato_uscita_3 = &stato_uscita[2];
	BinarySensor *stato_uscita_4 = &stato_uscita[3];
	BinarySensor *stato_uscita_5 = &stato_uscita[4];
	BinarySensor *stato_uscita_6 = &stato_uscita[5];
	BinarySensor *stato_uscita_7 = &stato_uscita[6];
	BinarySensor *stato_uscita_8 = &stato_uscita[7];
	
	BinarySensor *kyo_comunication = new BinarySensor("Kyo Comunication");

	BinarySensor *warn_mancanza_rete = new BinarySensor("Warning Mancanza Rete");
	BinarySensor *warn_scomparsa_bpi = new BinarySensor("Warning Scomparsa BPI");
	BinarySensor *warn_fusibile = new BinarySensor("Warning Fusibile");
	BinarySensor *warn_batteria_bassa = new BinarySensor("Warning Batteria Bassa");
	BinarySensor *warn_guasto_linea_telefonica = new BinarySensor("Warning Guasto Linea Telefonica");
	BinarySensor *warn_codici_default = new BinarySensor("Warning Codici Default");
	BinarySensor *warn_wireless = new BinarySensor("Warning Wireless");

	BinarySensor *stato_sirena = new BinarySensor("stato sirena");
	BinarySensor *sabotaggio_zona = new BinarySensor("Sabotaggio di Zona");
	BinarySensor *sabotaggio_chiave_falsa = new BinarySensor("Sabotaggio Chiave Falsa");
	BinarySensor *sabotaggio_bpi = new BinarySensor("Sabotaggio BPI");
	BinarySensor *sabotaggio_sistema = new BinarySensor("Sabotaggio Sistema");
	BinarySensor *sabotaggio_jam = new BinarySensor("Sabotaggio JAM");
	BinarySensor *sabotaggio_wireless = new BinarySensor("Sabotaggio Wireless");

	byte cmdGetSensorStatus[6] = {0xf0, 0x04, 0xf0, 0x0a, 0x00, 0xee};	  // Read Realtime Status and Trouble Status
	byte cmdGetPartitionStatus[6] = {0xf0, 0x02, 0x15, 0x12, 0x00, 0x19}; // Partitions Status (305) - Outputs Status - Tamper Memory - Bypassed Zones - Zone Alarm Memory - Zone Tamper Memory
	byte cmqGetSoftwareVersion[6] = {0xf0, 0x00, 0x00, 0x0b, 0x00, 0xfb}; // f0 00 00 0b 00 fb
	byte cmdResetAllarms[9] = {0x0F, 0x05, 0xF0, 0x01, 0x00, 0x05, 0xFF, 0x00, 0xFF};

	enum class PollingStateEnum { Init = 1, Status };

	bool serialTrace = false;
	bool logTrace = false;
	PollingStateEnum pollingState;
	int centralInvalidMessageCount = 0;

	int sendMessageToKyo(byte *cmd, int lcmd, byte ReadByes[], int waitForAnswer = 0)
	{
		// clean rx buffer
		while (available() > 0)
			read();

		delay(10);

		int index = 0;
		byte RxBuff[255];
		memset(ReadByes, 0, 254);

		write_array(cmd, lcmd);
		delay(waitForAnswer);
		
		// Read a single Byte
		while (available() > 0)
			RxBuff[index++] = read(); 

		if (this->serialTrace)
		{
			int i;
			char txString[255];
			char rxString[255];
			memset(txString, 0, 255);
			memset(rxString, 0, 255);

			for (i = 0; i < lcmd; i++)
				sprintf(txString, "%s %02X", txString, cmd[i]);

			for (i = 0; i < index; i++)
				sprintf(rxString, "%s %02X", rxString, RxBuff[i]);

			ESP_LOGD("sendMessageToKyo", "TX [%d] '%s', RX [%d] '%s'", lcmd, txString, index, rxString);
		}

		if (index <= 0)
			return -1;

		memcpy(ReadByes, RxBuff, index);
		return index;
	}

	void setup() override
	{
		register_service(&Bentel_Kyo32::arm_area, "arm_area", {"area", "arm_type", "specific_area"});
		register_service(&Bentel_Kyo32::disarm_area, "disarm_area", {"area", "specific_area"});
		register_service(&Bentel_Kyo32::reset_alarms, "reset_alarms");
		register_service(&Bentel_Kyo32::activate_output, "activate_output", {"output_number"});
		register_service(&Bentel_Kyo32::deactivate_output, "deactivate_output", {"output_number"});
		register_service(&Bentel_Kyo32::debug_command, "debug_command", {"serial_trace", "log_trace"});

		//register_service(&Bentel_Kyo32::on_clock_setting, "clock_setting", {"pin", "day", "month", "year", "hour", "minutes", "seconds", "data_format"});
		//register_service(&Bentel_Kyo32::on_bypass_zone, "bypass_zone", {"pin", "zone_number"});
		//register_service(&Bentel_Kyo32::on_unbypass_zone, "unbypass_zone", {"pin", "zone_number"});

		this->pollingState = PollingStateEnum::Init;
		this->kyo_comunication->publish_state(false);
		this->set_update_interval(500);
	}

	byte calculateCRC(byte *cmd, int lcmd)
	{
		int sum = 0x00;
		for (int i = 0; i <= lcmd; i++)
			sum += cmd[i];

		return (0x203 - sum);
	}

	void arm_area(int area, int arm_type, int specific_area)
	{
		if (area > MAX_AREE)
		{
			ESP_LOGD("arm_area", "Invalid Area %i, MAX %i", area, MAX_AREE);
			return;
		}
		
		ESP_LOGD("arm_area", "request arm type %d area %d", arm_type, area);
		byte cmdArmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0xFF};

		byte total_insert_area_status = 0x00, partial_insert_area_status = 0x00;

		if (specific_area == 1)
		{
			for (int i = 0; i < MAX_AREE; i++)
			{
				total_insert_area_status |= (this->inserimento_totale_area[i].state) << i;
				partial_insert_area_status |= (this->inserimento_parziale_area[i].state) << i;
			}
		}

		if (arm_type == 2)
			partial_insert_area_status |= 1 << (area - 1);
		else
			total_insert_area_status |= 1 << (area - 1);

		cmdArmPartition[6] = total_insert_area_status;
		cmdArmPartition[7] = partial_insert_area_status;
		cmdArmPartition[9] = calculateCRC(cmdArmPartition, 8);

		byte Rx[255];
		int Count = sendMessageToKyo(cmdArmPartition, sizeof(cmdArmPartition), Rx, 100);
		ESP_LOGD("arm_area", "arm_area kyo respond %i", Count);
	}

	void disarm_area(int area, int specific_area)
	{
		if (area > MAX_AREE)
		{
			ESP_LOGD("arm_area", "invalid Area %i, MAX %i", area, MAX_AREE);
			return;
		}
		
		ESP_LOGD("disarm_area", "request disarm area %d", area);
		byte cmdDisarmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};

		byte total_insert_area_status = 0x00, partial_insert_area_status = 0x00;

		if (specific_area == 1)
		{
			for (int i = 0; i < MAX_AREE; i++)
			{
				total_insert_area_status |= (this->inserimento_totale_area[i].state) << i;
				partial_insert_area_status |= (this->inserimento_parziale_area[i].state) << i;
			}

			if (this->inserimento_totale_area[area - 1].state)
				total_insert_area_status |= 0 << (area - 1);
			else
				partial_insert_area_status |= 1 << (area - 1);
		}

		cmdDisarmPartition[6] = total_insert_area_status;
		cmdDisarmPartition[7] = partial_insert_area_status;
		cmdDisarmPartition[9] = calculateCRC(cmdDisarmPartition, 8);

		byte Rx[255];
		int Count = sendMessageToKyo(cmdDisarmPartition, sizeof(cmdDisarmPartition), Rx, 80);
		ESP_LOGD("disarm_area", "kyo respond %i", Count);
	}

	void reset_alarms()
	{
		ESP_LOGD("reset_alarms", "Reset Alarms.");

		byte Rx[255];
		int Count = sendMessageToKyo(cmdResetAllarms, sizeof(cmdResetAllarms), Rx, 80);
		ESP_LOGD("reset_alarms", "kyo respond %i", Count);
	}

	void debug_command(int serial_trace, int log_trace)
	{
		this->serialTrace = (serial_trace == 1);
		this->logTrace = (log_trace == 1);

		ESP_LOGD("debug_command", "serial_trace %i log_trace %i", this->serialTrace, this->logTrace);
	}

	void loop() override
	{
	}

	void activate_output(int output_number)
	{
		if (output_number > MAX_AREE)
		{
			ESP_LOGD("activate_output", "invalid output %i, MAX %i", output_number, MAX_AREE);
			return;
		}

		ESP_LOGD("activate_output", "activate Output Number: %d", output_number);
		
		byte cmdActivateOutput[9] = {0x0f, 0x06, 0xf0, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00};
		
		cmdActivateOutput[6] |= 1 << (output_number - 1);
		cmdActivateOutput[8] = cmdActivateOutput[6];

		byte Rx[255];
		int Count = sendMessageToKyo(cmdActivateOutput, sizeof(cmdActivateOutput), Rx, 80);
		ESP_LOGD("activate_output", "kyo respond %i", Count);
	}

	void deactivate_output(int output_number)
	{
		if (output_number > MAX_AREE)
		{
			ESP_LOGD("deactivate_output", "invalid output %i, MAX %i", output_number, MAX_AREE);
			return;
		}

		ESP_LOGD("deactivate_output", "deactivate Output Number: %d", output_number);
		
		byte cmdDeactivateOutput[9] = {0x0f, 0x06, 0xf0, 0x01, 0x00, 0x06, 0x00, 0x00, 0xCC};
		
		cmdDeactivateOutput[7] |= 1 << (output_number - 1);
		cmdDeactivateOutput[8] = cmdDeactivateOutput[7];

		byte Rx[255];
		int Count = sendMessageToKyo(cmdDeactivateOutput, sizeof(cmdDeactivateOutput), Rx, 80);
		ESP_LOGD("deactivate_output", "kyo respond %i", Count);
	}

	/*
	void on_bypass_zone(int pin, int zone_number)
	{
		ESP_LOGD("custom", "Bypass Zone. PIN: %d, Zone Number: %d", pin, zone_number);
	}

	void on_unbypass_zone(int pin, int zone_number)
	{
		ESP_LOGD("custom", "UnBypass Zone. PIN: %d, Zone Number: %d", pin, zone_number);
	}

	void on_clock_setting(int pin, int day, int month, int year, int hour, int minutes, int seconds, int data_format)
	{
		ESP_LOGD("custom", "Clock Setting. PIN: %d, Day: %d, Month: %d, Year: %d, Hour: %d, Minutes: %d, Seconds: %d, Data Format: %d", pin, day, month, year, hour, minutes, seconds, data_format);
	}
	*/

	void update() override
	{
		switch(this->pollingState)
		{
			case PollingStateEnum::Init:
				if (this->update_kyo_status())
				{
					this->pollingState = PollingStateEnum::Status;
					this->centralInvalidMessageCount = 0;
				}
				else
				{
					this->centralInvalidMessageCount++;
				}

				break;

			case PollingStateEnum::Status:
				if (this->update_kyo_partitions())
				{
					this->pollingState = PollingStateEnum::Init;
					this->centralInvalidMessageCount = 0;
				}
				else
					this->centralInvalidMessageCount++;
		
				break;
		}

		if (this->centralInvalidMessageCount == 0 && !this->kyo_comunication->state)
			this->kyo_comunication->publish_state(true);
		else if(centralInvalidMessageCount > 3)
			this->kyo_comunication->publish_state(false);
	}

	bool update_kyo_partitions()
	{
		byte Rx[255];
		int Count = 0;

		Count = sendMessageToKyo(cmdGetPartitionStatus, sizeof(cmdGetPartitionStatus), Rx, 100);
		if (Count != 26)
		{
			if (this->logTrace)
				ESP_LOGD("update_kyo_partitions", "invalid message length %i", Count);

			return (false);
		}

		int StatoZona, i;

		// Ciclo AREE INSERITE
		for (i = 0; i < MAX_AREE; i++)
		{
			StatoZona = (Rx[6] >> i) & 1;
			if (this->logTrace && (StatoZona == 1) != inserimento_totale_area[i].state)
				ESP_LOGD("aree_totale", "Area %i - Stato %i", i, StatoZona);
			
			inserimento_totale_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo AREE INSERITE PARZIALI
		for (i = 0; i < MAX_AREE; i++)
		{
			StatoZona = (Rx[7] >> i) & 1;
			if (this->logTrace && (StatoZona == 1) != inserimento_parziale_area[i].state)
				ESP_LOGD("aree_parziale", "Area %i - Stato %i", i, StatoZona);

			inserimento_parziale_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo AREE INSERITE PARZIALI RITARDO 0
		for (i = 0; i < MAX_AREE; i++)
		{
			StatoZona = (Rx[8] >> i) & 1;
			if (this->logTrace && (StatoZona == 1) != inserimento_parziale_ritardo_0_area[i].state)
				ESP_LOGD("inserimento_parziale_ritardo_0_area", "Area %i - Stato %i", i, StatoZona);

			inserimento_parziale_ritardo_0_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo AREE DISINSERITE
		for (i = 0; i < MAX_AREE; i++)
		{
			StatoZona = (Rx[9] >> i) & 1;
			if (this->logTrace && (StatoZona == 1) != disinserita_area[i].state)
				ESP_LOGD("disinserita_area", "Area %i - Stato %i", i, StatoZona);

			disinserita_area[i].publish_state(StatoZona == 1);
		}

		// STATO SIRENA
		stato_sirena->publish_state(((Rx[10] >> 5) & 1) == 1);

		// CICLO STATO USCITE
		for (i = 0; i < MAX_AREE; i++)
		{
			StatoZona = 0;
			if (i >= 8 && i <= 15)
				StatoZona = (Rx[11] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[12] >> i) & 1;

			if (this->logTrace && (StatoZona == 1) != stato_uscita[i].state)
				ESP_LOGD("stato_uscita", "Uscita %i - Stato %i", i, StatoZona);

			stato_uscita[i].publish_state(StatoZona == 1);
		}

		// CICLO ZONE ESCLUSE
		for (i = 0; i < MAX_ZONE; i++)
		{
			StatoZona = 0;
			if (i >= 24)
				StatoZona = (Rx[13] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[14] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[15] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[16] >> i) & 1;

			zona_esclusa[i].publish_state(StatoZona == 1);
		}

		// CICLO MEMORIA ALLARME ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			StatoZona = 0;
			if (i >= 24)
				StatoZona = (Rx[17] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[18] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[19] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[20] >> i) & 1;

			if (this->logTrace && (StatoZona == 1) != memoria_allarme_zona[i].state)	
				ESP_LOGD("memoria_allarme_zona", "Zona %i - Stato %i", i, StatoZona);

			memoria_allarme_zona[i].publish_state(StatoZona == 1);
		}

		// CICLO MEMORIA SABOTAGGIO ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			StatoZona = 0;
			if (i >= 24)
				StatoZona = (Rx[21] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[22] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[23] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[24] >> i) & 1;

			if (this->logTrace && (StatoZona == 1) != memoria_sabotaggio_zona[i].state)	
				ESP_LOGD("memoria_sabotaggio_zona", "Zona %i - Stato %i", i, StatoZona);

			memoria_sabotaggio_zona[i].publish_state(StatoZona == 1);
		}
		
		return true;
	}

	bool update_kyo_status()
	{
		byte Rx[255];
		int Count = 0;

		Count = sendMessageToKyo(cmdGetSensorStatus, sizeof(cmdGetSensorStatus), Rx, 100);
		if (Count != 18)
		{
			if (this->logTrace)
				ESP_LOGD("update_kyo_status", "invalid message length %i", Count);
			
			return false;
		}

		int StatoZona, i;

		// Ciclo ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			StatoZona = 0;
			if (i >= 24)
				StatoZona = (Rx[6] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[7] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[8] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[9] >> i) & 1;

			//ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
			zona[i].publish_state(StatoZona==1);
		}

		// Ciclo SABOTAGGIO ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			StatoZona = 0;
			if (i >= 24)
				StatoZona = (Rx[10] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[11] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[12] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[13] >> i) & 1;

			zona_sabotaggio[i].publish_state(StatoZona == 1);
		}

		// Ciclo ALLARME AREA
		for (i = 0; i < MAX_AREE; i++)
		{
			StatoZona = (Rx[15] >> i) & 1;
			allarme_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo WARNINGS
		for (i = 0; i < 8; i++)
		{
			StatoZona = (Rx[14] >> i) & 1;
			switch(i)
			{
				case 0:
					warn_mancanza_rete->publish_state(StatoZona == 1);
					break;

				case 1:
					warn_scomparsa_bpi->publish_state(StatoZona == 1);
					break;

				case 2:
					warn_fusibile->publish_state(StatoZona == 1);
					break;

				case 3:
					warn_batteria_bassa->publish_state(StatoZona == 1);
					break;

				case 4:
					warn_guasto_linea_telefonica->publish_state(StatoZona == 1);
					break;

				case 5:
					warn_codici_default->publish_state(StatoZona == 1);
					break;

				case 6:
					warn_wireless->publish_state(StatoZona == 1);
					break;
			}
		}

		// Ciclo SABOTAGGI
		for (i = 0; i < 8; i++)
		{
			StatoZona = (Rx[16] >> i) & 1;
			switch(i)
			{
				case 2:
					sabotaggio_zona->publish_state(StatoZona == 1);
					break;

				case 3:
					sabotaggio_chiave_falsa->publish_state(StatoZona == 1);
					break;

				case 4:
					sabotaggio_bpi->publish_state(StatoZona == 1);
					break;

				case 5:
					sabotaggio_sistema->publish_state(StatoZona == 1);
					break;
				
				case 6:
					sabotaggio_jam->publish_state(StatoZona == 1);
					break;

				case 7:
					sabotaggio_wireless->publish_state(StatoZona == 1);
					break;
			}
		}

		return true;
	}
};
