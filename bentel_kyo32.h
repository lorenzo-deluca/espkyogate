#include "esphome.h"

#define MAX_ZONE 32
#define MAX_AREE 8

class Bentel_Kyo32 : public PollingComponent, public UARTDevice, public CustomAPIDevice
{
public:
	Bentel_Kyo32(UARTComponent *parent) : UARTDevice(parent) {}

	// mapping zones array for lambda return
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

	// mapping zona_sabotaggio array for lambda return
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
	BinarySensor *zona_esclusa_1 = &zona_sabotaggio[0];
	BinarySensor *zona_esclusa_2 = &zona_sabotaggio[1];
	BinarySensor *zona_esclusa_3 = &zona_sabotaggio[2];
	BinarySensor *zona_esclusa_4 = &zona_sabotaggio[3];
	BinarySensor *zona_esclusa_5 = &zona_sabotaggio[4];
	BinarySensor *zona_esclusa_6 = &zona_sabotaggio[5];
	BinarySensor *zona_esclusa_7 = &zona_sabotaggio[6];
	BinarySensor *zona_esclusa_8 = &zona_sabotaggio[7];
	
	BinarySensor* memoria_allarme_zona = new BinarySensor[MAX_ZONE];
	BinarySensor *memoria_allarme_zona_1 = &memoria_allarme_zona[0];
	BinarySensor *memoria_allarme_zona_2 = &memoria_allarme_zona[1];
	BinarySensor *memoria_allarme_zona_3 = &memoria_allarme_zona[2];
	BinarySensor *memoria_allarme_zona_4 = &memoria_allarme_zona[3];
	BinarySensor *memoria_allarme_zona_5 = &memoria_allarme_zona[4];
	BinarySensor *memoria_allarme_zona_6 = &memoria_allarme_zona[5];
	BinarySensor *memoria_allarme_zona_7 = &memoria_allarme_zona[6];
	BinarySensor *memoria_allarme_zona_8 = &memoria_allarme_zona[7];
	
	BinarySensor* memoria_sabotaggio_zona = new BinarySensor[MAX_ZONE];
	BinarySensor *memoria_sabotaggio_zona_1 = &memoria_sabotaggio_zona[0];
	BinarySensor *memoria_sabotaggio_zona_2 = &memoria_sabotaggio_zona[1];
	BinarySensor *memoria_sabotaggio_zona_3 = &memoria_sabotaggio_zona[2];
	BinarySensor *memoria_sabotaggio_zona_4 = &memoria_sabotaggio_zona[3];
	BinarySensor *memoria_sabotaggio_zona_5 = &memoria_sabotaggio_zona[4];
	BinarySensor *memoria_sabotaggio_zona_6 = &memoria_sabotaggio_zona[5];
	BinarySensor *memoria_sabotaggio_zona_7 = &memoria_sabotaggio_zona[6];
	BinarySensor *memoria_sabotaggio_zona_8 = &memoria_sabotaggio_zona[7];
	
	BinarySensor *warn_mancanza_rete = new BinarySensor("Warning Mancanza Rete");
	BinarySensor *warn_scomparsa_bpi = new BinarySensor("Warning Scomparsa BPI");
	BinarySensor *warn_fusibile = new BinarySensor("Warning Fusibile");
	BinarySensor *warn_batteria_bassa = new BinarySensor("Warning Batteria Bassa");
	BinarySensor *warn_guasto_linea_telefonica = new BinarySensor("Warning Guasto Linea Telefonica");
	BinarySensor *warn_codici_default = new BinarySensor("Warning Codici Default");
	BinarySensor *warn_wireless = new BinarySensor("Warning Wireless");

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

	BinarySensor *stato_sirena = new BinarySensor("stato sirena");
	BinarySensor *sabotaggio_zona = new BinarySensor("Sabotaggio di Zona");
	BinarySensor *sabotaggio_chiave_falsa = new BinarySensor("Sabotaggio Chiave Falsa");
	BinarySensor *sabotaggio_bpi = new BinarySensor("Sabotaggio BPI");
	BinarySensor *sabotaggio_sistema = new BinarySensor("Sabotaggio Sistema");
	BinarySensor *sabotaggio_jam = new BinarySensor("Sabotaggio JAM");
	BinarySensor *sabotaggio_wireless = new BinarySensor("Sabotaggio Wireless");

	byte cmdGetSensorStatus[6] = {0xf0, 0x04, 0xf0, 0x0a, 0x00, 0xee};	  // f0 04 f0 0a 00 ee - Read Realtime Status and Trouble Status
	byte cmdGetPartitionStatus[6] = {0xf0, 0x02, 0x15, 0x12, 0x00, 0x19}; // f0 02 15 12 00 19 - Partitions Status - Outputs Status - Tamper Memory - Bypassed Zones - Zone Alarm Memory - Zone Tamper Memory
	byte cmqGetSoftwareVersion[6] = {0xf0, 0x00, 0x00, 0x0b, 0x00, 0xfb}; // f0 00 00 0b 00 fb

	int sendMessageToKyo(byte *cmd, int lcmd, byte ReadByes[])
	{
		byte RxBuff[255];
		char TraceString[255];

		int index = 0, i = 0;
		int val;
		memset(ReadByes, 0, 254);

		write_array(cmd, lcmd);
		//delay(100);
		while (available() > 0)
		{
			// Read a single Byte
			RxBuff[index++] = read(); 
			//delay(3);
		}

		if (index > 0)
		{
			memcpy(ReadByes, RxBuff, index);
			return index;
		}
		else
			return -1;
	}

	void setup() override
	{
		register_service(&Bentel_Kyo32::on_clock_setting, "clock_setting",
						 {"pin", "day", "month", "year", "hour", "minutes", "seconds", "data_format"});

		register_service(&Bentel_Kyo32::arm_partitions, "arm_partitions",
						 {"pin", "partition_mask", "type"});

		register_service(&Bentel_Kyo32::disarm_partitions, "disarm_partitions",
						 {"pin", "partition_mask", "type"});

		register_service(&Bentel_Kyo32::on_reset_alarms, "reset_alarms",
						 {"pin"});

		register_service(&Bentel_Kyo32::on_bypass_zone, "bypass_zone",
						 {"pin", "zone_number"});

		register_service(&Bentel_Kyo32::on_unbypass_zone, "unbypass_zone",
						 {"pin", "zone_number"});

		register_service(&Bentel_Kyo32::on_activate_output, "activate_output",
						 {"pin", "output_number"});

		register_service(&Bentel_Kyo32::on_deactivate_output, "deactivate_output",
						 {"pin", "output_number"});

		this->set_update_interval(250);
	}

	void on_clock_setting(int pin, int day, int month, int year, int hour, int minutes, int seconds, int data_format)
	{
		ESP_LOGD("custom", "Clock Setting. PIN: %d, Day: %d, Month: %d, Year: %d, Hour: %d, Minutes: %d, Seconds: %d, Data Format: %d", pin, day, month, year, hour, minutes, seconds, data_format);
	}

	void arm_partitions(int pin, int partition_mask, int type)
	{
		ESP_LOGD("custom", "Arm Partitions. PIN: %d, Partition Mask: %d, Type: %d", pin, partition_mask, type);

		byte cmdArmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x01, 0x00, 0x00, 0xFE, 0xFF};

		byte Rx[255];
		int Count = sendMessageToKyo(cmdArmPartition, sizeof(cmdArmPartition), Rx);
		ESP_LOGD("custom", "arm_partitions kyo respond %i", Count);
	}

	void disarm_partitions(int pin, int partition_mask, int type)
	{
		ESP_LOGD("custom", "Disarm Partitions. PIN: %d, Partition Mask: %d, Type: %d", pin, partition_mask, type);
		byte cmdDisarmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};

		byte Rx[255];
		int Count = sendMessageToKyo(cmdDisarmPartition, sizeof(cmdDisarmPartition), Rx);
		ESP_LOGD("custom", "disarm_partitions kyo respond %i", Count);
	}

	void on_reset_alarms(int pin)
	{
		ESP_LOGD("custom", "Reset Alarms. PIN: %d", pin);
	}

	void on_bypass_zone(int pin, int zone_number)
	{
		ESP_LOGD("custom", "Bypass Zone. PIN: %d, Zone Number: %d", pin, zone_number);
	}

	void on_unbypass_zone(int pin, int zone_number)
	{
		ESP_LOGD("custom", "UnBypass Zone. PIN: %d, Zone Number: %d", pin, zone_number);
	}

	void on_activate_output(int pin, int output_number)
	{
		ESP_LOGD("custom", "Activate Output. PIN: %d, Output Number: %d", pin, output_number);
	}

	void on_deactivate_output(int pin, int output_number)
	{
		ESP_LOGD("custom", "Deactivate Output. PIN: %d, Output Number: %d", pin, output_number);
	}

	void loop() override
	{
	}

	void update() override
	{
		this->update_sensor_status();
		//delay(100);
		this->update_aree_status();
		//delay(100);
	}

	void update_aree_status()
	{
		byte Rx[255];
		int Count = 0, i;

		int StatoZona;
		byte mask = 1;

		Count = sendMessageToKyo(cmdGetPartitionStatus, sizeof(cmdGetPartitionStatus), Rx);
		if (Count != 26)
		{
			//kyo_comunication->publish_state(false);
			//ESP_LOGD("custom", "update_aree_status invalid message length %i", Count);
			return;
		}

		kyo_comunication->publish_state(true);

		// Ciclo AREE INSERITE
		for (i = 0; i < 8; i++)
		{
			mask = 00000001;
			StatoZona = -1;
			StatoZona = (Rx[6] >> i) & 1;
			inserimento_totale_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo AREE INSERITE PARZIALI
		for (i = 0; i < MAX_AREE; i++)
		{
			mask = 00000001;
			StatoZona = -1;
			StatoZona = (Rx[7] >> i) & 1;

			inserimento_parziale_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo AREE INSERITE PARZIALI RITARDO 0
		for (i = 0; i < MAX_AREE; i++)
		{
			mask = 00000001;
			StatoZona = -1;
			StatoZona = (Rx[8] >> i) & 1;

			inserimento_parziale_ritardo_0_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo AREE DISINSERITE
		for (i = 0; i < MAX_AREE; i++)
		{
			mask = 00000001;
			StatoZona = -1;
			StatoZona = (Rx[9] >> i) & 1;

			disinserita_area[i].publish_state(StatoZona == 1);
		}

		// Ciclo STATO SIRENA
		for (i = 0; i < MAX_AREE; i++)
		{
			mask = 00000001;
			StatoZona = -1;
			StatoZona = (Rx[10] >> i) & 1;

			if (i == 0)
			{
				stato_sirena->publish_state(StatoZona == 1);
				break;
			}
		}

		// CICLO STATO USCITE
		for (i = 0; i < MAX_ZONE; i++)
		{
			mask = 00000001;
			StatoZona = -1;

			if (i >= 8 && i <= 15)
				StatoZona = (Rx[11] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[12] >> i) & 1;

			//ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
			stato_uscita[i].publish_state(StatoZona == 1);
		}

		// CICLO ZONE ESCLUSE
		for (i = 0; i < MAX_ZONE; i++)
		{
			mask = 00000001;
			StatoZona = -1;

			if (i >= 24)
				StatoZona = (Rx[13] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[14] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[15] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[16] >> i) & 1;

			//ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
			zona_esclusa[i].publish_state(StatoZona == 1);
		}

		// CICLO MEMORIA ALLARME ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			mask = 00000001;
			StatoZona = -1;

			if (i >= 24)
				StatoZona = (Rx[17] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[18] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[19] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[20] >> i) & 1;

			//ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
			memoria_allarme_zona[i].publish_state(StatoZona == 1);
		}

		// CICLO MEMORIA SABOTAGGIO ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			mask = 00000001;
			StatoZona = -1;

			if (i >= 24)
				StatoZona = (Rx[21] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[22] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[23] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[24] >> i) & 1;

			//ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
			memoria_sabotaggio_zona[i].publish_state(StatoZona == 1);
		}
	}

	void update_sensor_status()
	{
		byte Rx[255];
		int Count = 0, i;

		int StatoZona;
		byte mask = 1;

		Count = sendMessageToKyo(cmdGetSensorStatus, sizeof(cmdGetSensorStatus), Rx);
		if (Count != 18)
		{
			//ESP_LOGD("custom", "update_sensor_status invalid message length %i", Count);
			return;
		}

		// Ciclo ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			mask = 00000001;
			StatoZona = -1;

			if (i >= 24)
				StatoZona = (Rx[6] >> (i - 24)) & 1;
			else if (i >= 16 && i <= 23)
				StatoZona = (Rx[7] >> (i - 16)) & 1;
			else if (i >= 8 && i <= 15)
				StatoZona = (Rx[8] >> (i - 8)) & 1;
			else if (i <= 7)
				StatoZona = (Rx[9] >> i) & 1;

			//ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
			zona[i].publish_state((StatoZona==1));
		}

		// Ciclo SABOTAGGIO ZONE
		for (i = 0; i < MAX_ZONE; i++)
		{
			mask = 00000001;
			StatoZona = -1;

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

		// Ciclo WARNINGS
		for (i = 0; i < 8; i++)
		{
			mask = 00000001;
			StatoZona = -1;
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

		// Ciclo ALLARME AREA
		for (i = 0; i < MAX_AREE; i++)
		{
			mask = 00000001;
			StatoZona = -1;
			StatoZona = (Rx[15] >> i) & 1;

			allarme_area[0].publish_state(StatoZona == 1);
		}

		// Ciclo SABOTAGGI
		for (i = 0; i < 8; i++)
		{
			mask = 00000001;
			StatoZona = -1;
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
	}
};
