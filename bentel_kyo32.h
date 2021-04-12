#include "esphome.h"

#define N_ZONE 32

class Bentel_Kyo32 : public PollingComponent, public UARTDevice, public CustomAPIDevice
{
public:
  Bentel_Kyo32(UARTComponent *parent) : UARTDevice(parent) {}

  BinarySensor *zona_1 = new BinarySensor();
  BinarySensor *zona_2 = new BinarySensor();
  BinarySensor *zona_3 = new BinarySensor();
  BinarySensor *zona_4 = new BinarySensor();
  BinarySensor *zona_5 = new BinarySensor();
  BinarySensor *zona_6 = new BinarySensor();
  BinarySensor *zona_7 = new BinarySensor();
  BinarySensor *zona_8 = new BinarySensor();
  BinarySensor *zona_9 = new BinarySensor();  
  BinarySensor *zona_10 = new BinarySensor();
  BinarySensor *zona_11 = new BinarySensor();
  BinarySensor *zona_12 = new BinarySensor();
  BinarySensor *zona_13 = new BinarySensor();
  BinarySensor *zona_14 = new BinarySensor();
  BinarySensor *zona_15 = new BinarySensor();
  BinarySensor *zona_16 = new BinarySensor();
  BinarySensor *zona_17 = new BinarySensor();
  BinarySensor *zona_18 = new BinarySensor();
  BinarySensor *zona_19 = new BinarySensor();
  BinarySensor *zona_20 = new BinarySensor();
  BinarySensor *zona_21 = new BinarySensor();
  BinarySensor *zona_22 = new BinarySensor();
  BinarySensor *zona_23 = new BinarySensor();
  BinarySensor *zona_24 = new BinarySensor();
  BinarySensor *zona_25 = new BinarySensor();
  BinarySensor *zona_26 = new BinarySensor();
  BinarySensor *zona_27 = new BinarySensor();
  BinarySensor *zona_28 = new BinarySensor();
  BinarySensor *zona_29 = new BinarySensor();
  BinarySensor *zona_30 = new BinarySensor();
  BinarySensor *zona_31 = new BinarySensor();
  BinarySensor *zona_32 = new BinarySensor();

  BinarySensor *zona_sabotaggio_1 = new BinarySensor();
  BinarySensor *zona_sabotaggio_2 = new BinarySensor();
  BinarySensor *zona_sabotaggio_3 = new BinarySensor();
  BinarySensor *zona_sabotaggio_4 = new BinarySensor();
  BinarySensor *zona_sabotaggio_5 = new BinarySensor();
  BinarySensor *zona_sabotaggio_6 = new BinarySensor();
  BinarySensor *zona_sabotaggio_7 = new BinarySensor();
  BinarySensor *zona_sabotaggio_8 = new BinarySensor();
  BinarySensor *zona_sabotaggio_9 = new BinarySensor();
  BinarySensor *zona_sabotaggio_10 = new BinarySensor();
  BinarySensor *zona_sabotaggio_11 = new BinarySensor();
  BinarySensor *zona_sabotaggio_12 = new BinarySensor();
  BinarySensor *zona_sabotaggio_13 = new BinarySensor();
  BinarySensor *zona_sabotaggio_14 = new BinarySensor();
  BinarySensor *zona_sabotaggio_15 = new BinarySensor();
  BinarySensor *zona_sabotaggio_16 = new BinarySensor();
  BinarySensor *zona_sabotaggio_17 = new BinarySensor();
  BinarySensor *zona_sabotaggio_18 = new BinarySensor();
  BinarySensor *zona_sabotaggio_19 = new BinarySensor();
  BinarySensor *zona_sabotaggio_20 = new BinarySensor();
  BinarySensor *zona_sabotaggio_21 = new BinarySensor();
  BinarySensor *zona_sabotaggio_22 = new BinarySensor();
  BinarySensor *zona_sabotaggio_23 = new BinarySensor();
  BinarySensor *zona_sabotaggio_24 = new BinarySensor();
  BinarySensor *zona_sabotaggio_25 = new BinarySensor();
  BinarySensor *zona_sabotaggio_26 = new BinarySensor();
  BinarySensor *zona_sabotaggio_27 = new BinarySensor();
  BinarySensor *zona_sabotaggio_28 = new BinarySensor();
  BinarySensor *zona_sabotaggio_29 = new BinarySensor();
  BinarySensor *zona_sabotaggio_30 = new BinarySensor();
  BinarySensor *zona_sabotaggio_31 = new BinarySensor();
  BinarySensor *zona_sabotaggio_32 = new BinarySensor();

  BinarySensor *zona_esclusa_1 = new BinarySensor();
  BinarySensor *zona_esclusa_2 = new BinarySensor();
  BinarySensor *zona_esclusa_3 = new BinarySensor();
  BinarySensor *zona_esclusa_4 = new BinarySensor();
  BinarySensor *zona_esclusa_5 = new BinarySensor();
  BinarySensor *zona_esclusa_6 = new BinarySensor();
  BinarySensor *zona_esclusa_7 = new BinarySensor();
  BinarySensor *zona_esclusa_8 = new BinarySensor();
  BinarySensor *zona_esclusa_9 = new BinarySensor();  
  BinarySensor *zona_esclusa_10 = new BinarySensor();
  BinarySensor *zona_esclusa_11 = new BinarySensor();
  BinarySensor *zona_esclusa_12 = new BinarySensor();
  BinarySensor *zona_esclusa_13 = new BinarySensor();
  BinarySensor *zona_esclusa_14 = new BinarySensor();
  BinarySensor *zona_esclusa_15 = new BinarySensor();
  BinarySensor *zona_esclusa_16 = new BinarySensor();
  BinarySensor *zona_esclusa_17 = new BinarySensor();
  BinarySensor *zona_esclusa_18 = new BinarySensor();
  BinarySensor *zona_esclusa_19 = new BinarySensor();
  BinarySensor *zona_esclusa_20 = new BinarySensor();
  BinarySensor *zona_esclusa_21 = new BinarySensor();
  BinarySensor *zona_esclusa_22 = new BinarySensor();
  BinarySensor *zona_esclusa_23 = new BinarySensor();
  BinarySensor *zona_esclusa_24 = new BinarySensor();
  BinarySensor *zona_esclusa_25 = new BinarySensor();
  BinarySensor *zona_esclusa_26 = new BinarySensor();
  BinarySensor *zona_esclusa_27 = new BinarySensor();
  BinarySensor *zona_esclusa_28 = new BinarySensor();
  BinarySensor *zona_esclusa_29 = new BinarySensor();
  BinarySensor *zona_esclusa_30 = new BinarySensor();
  BinarySensor *zona_esclusa_31 = new BinarySensor();
  BinarySensor *zona_esclusa_32 = new BinarySensor();

  BinarySensor *memoria_allarme_zona_1 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_2 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_3 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_4 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_5 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_6 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_7 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_8 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_9 = new BinarySensor();  
  BinarySensor *memoria_allarme_zona_10 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_11 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_12 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_13 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_14 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_15 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_16 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_17 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_18 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_19 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_20 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_21 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_22 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_23 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_24 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_25 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_26 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_27 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_28 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_29 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_30 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_31 = new BinarySensor();
  BinarySensor *memoria_allarme_zona_32 = new BinarySensor();

  BinarySensor *memoria_sabotaggio_zona_1 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_2 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_3 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_4 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_5 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_6 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_7 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_8 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_9 = new BinarySensor();  
  BinarySensor *memoria_sabotaggio_zona_10 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_11 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_12 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_13 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_14 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_15 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_16 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_17 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_18 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_19 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_20 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_21 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_22 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_23 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_24 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_25 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_26 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_27 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_28 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_29 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_30 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_31 = new BinarySensor();
  BinarySensor *memoria_sabotaggio_zona_32 = new BinarySensor();

  BinarySensor *warn_mancanza_rete = new BinarySensor("Warning Mancanza Rete");
  BinarySensor *warn_scomparsa_bpi = new BinarySensor("Warning Scomparsa BPI");;
  BinarySensor *warn_fusibile = new BinarySensor("Warning Fusibile");;
  BinarySensor *warn_batteria_bassa = new BinarySensor("Warning Batteria Bassa");;
  BinarySensor *warn_guasto_linea_telefonica = new BinarySensor("Warning Guasto Linea Telefonica");;
  BinarySensor *warn_codici_default = new BinarySensor("Warning Codici Default");;
  BinarySensor *warn_wireless = new BinarySensor("Warning Wireless");;

  BinarySensor *allarme_area_1 = new BinarySensor();
  BinarySensor *allarme_area_2 = new BinarySensor();
  BinarySensor *allarme_area_3 = new BinarySensor();
  BinarySensor *allarme_area_4 = new BinarySensor();
  BinarySensor *allarme_area_5 = new BinarySensor();
  BinarySensor *allarme_area_6 = new BinarySensor();
  BinarySensor *allarme_area_7 = new BinarySensor();
  BinarySensor *allarme_area_8 = new BinarySensor();

  BinarySensor *inserimento_totale_area_1 = new BinarySensor();
  BinarySensor *inserimento_totale_area_2 = new BinarySensor();
  BinarySensor *inserimento_totale_area_3 = new BinarySensor();
  BinarySensor *inserimento_totale_area_4 = new BinarySensor();
  BinarySensor *inserimento_totale_area_5 = new BinarySensor();
  BinarySensor *inserimento_totale_area_6 = new BinarySensor();
  BinarySensor *inserimento_totale_area_7 = new BinarySensor();
  BinarySensor *inserimento_totale_area_8 = new BinarySensor();

  BinarySensor *inserimento_parziale_area_1 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_2 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_3 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_4 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_5 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_6 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_7 = new BinarySensor();
  BinarySensor *inserimento_parziale_area_8 = new BinarySensor();

  BinarySensor *inserimento_parziale_ritardo_0_area_1 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_2 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_3 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_4 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_5 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_6 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_7 = new BinarySensor();
  BinarySensor *inserimento_parziale_ritardo_0_area_8 = new BinarySensor();

  BinarySensor *disinserita_area_1 = new BinarySensor();
  BinarySensor *disinserita_area_2 = new BinarySensor();
  BinarySensor *disinserita_area_3 = new BinarySensor();
  BinarySensor *disinserita_area_4 = new BinarySensor();
  BinarySensor *disinserita_area_5 = new BinarySensor();
  BinarySensor *disinserita_area_6 = new BinarySensor();
  BinarySensor *disinserita_area_7 = new BinarySensor();
  BinarySensor *disinserita_area_8 = new BinarySensor();

  BinarySensor *stato_uscita_1 = new BinarySensor();
  BinarySensor *stato_uscita_2 = new BinarySensor();
  BinarySensor *stato_uscita_3 = new BinarySensor();
  BinarySensor *stato_uscita_4 = new BinarySensor();
  BinarySensor *stato_uscita_5 = new BinarySensor();
  BinarySensor *stato_uscita_6 = new BinarySensor();
  BinarySensor *stato_uscita_7 = new BinarySensor();
  BinarySensor *stato_uscita_8 = new BinarySensor();
  BinarySensor *stato_uscita_9 = new BinarySensor();  
  BinarySensor *stato_uscita_10 = new BinarySensor();
  BinarySensor *stato_uscita_11 = new BinarySensor();
  BinarySensor *stato_uscita_12 = new BinarySensor();
  BinarySensor *stato_uscita_13 = new BinarySensor();
  BinarySensor *stato_uscita_14 = new BinarySensor();
  BinarySensor *stato_uscita_15 = new BinarySensor();
  BinarySensor *stato_uscita_16 = new BinarySensor();

  BinarySensor *stato_sirena = new BinarySensor();

  BinarySensor *sabotaggio_zona = new BinarySensor("Sabotaggio di Zona");;
  BinarySensor *sabotaggio_chiave_falsa = new BinarySensor("Sabotaggio Chiave Falsa");;
  BinarySensor *sabotaggio_bpi = new BinarySensor("Sabotaggio BPI");;
  BinarySensor *sabotaggio_sistema = new BinarySensor("Sabotaggio Sistema");;
  BinarySensor *sabotaggio_jam = new BinarySensor("Sabotaggio JAM");;
  BinarySensor *sabotaggio_wireless = new BinarySensor("Sabotaggio Wireless");;

  byte cmdGetSensorStatus[6] = {0xf0, 0x04, 0xf0, 0x0a, 0x00, 0xee}; // f0 04 f0 0a 00 ee - Read Realtime Status and Trouble Status
  byte cmdGetPartitionStatus[6] = {0xf0, 0x02, 0x15, 0x12, 0x00, 0x19}; // f0 02 15 12 00 19 - Partitions Status - Outputs Status - Tamper Memory - Bypassed Zones - Zone Alarm Memory - Zone Tamper Memory
  byte cmqGetSoftwareVersion[6] = {0xf0, 0x00, 0x00, 0x0b, 0x00, 0xfb}; // f0 00 00 0b 00 fb

  bool getParm(byte *cmd, int lcmd, byte ReadByes[], int &CountBytes)
  {
    byte RxBuff[255];
    char TraceString[255];

    int index = 0, i = 0;

    int val;
    memset(ReadByes, 0, CountBytes);

    write_array(cmd, lcmd);
    //delay(100);
    while (available() > 0)
    {
      RxBuff[index++] = read(); // Read a Byte
      //delay(3);
    }

    if (index > 0)
    {
      CountBytes = index;
      memcpy(ReadByes, RxBuff, CountBytes);

      return true;
    }
    else
      return false;
  }

  void setup() override
  {
    register_service(&Bentel_Kyo32::on_clock_setting, "clock_setting",
                     {"pin", "day", "month", "year", "hour", "minutes", "seconds", "data_format"});

    register_service(&Bentel_Kyo32::on_arm_partitions, "arm_partitions",
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

  void on_arm_partitions(int pin, int partition_mask, int type)
  {
    ESP_LOGD("custom", "Arm Partitions. PIN: %d, Partition Mask: %d, Type: %d", pin, partition_mask, type);
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
    this->update_aree_status  ();
    //delay(100);
  }

  void update_aree_status()
  {
    bool val;
    byte Rx[255];
    int Count = 0, i;

    int StatoZona;
    byte mask = 1;

    val = getParm(cmdGetPartitionStatus, sizeof(cmdGetPartitionStatus), Rx, Count);

    if(Count == 26)
    {
      // Ciclo AREE INSERITE
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[6] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 0)
        {
          inserimento_totale_area_1->publish_state(val);
        }
        else if(i == 1)
        {
          inserimento_totale_area_2->publish_state(val);
        }
        else if(i == 2)
        {
          inserimento_totale_area_3->publish_state(val);
        }
        else if(i == 3)
        {
          inserimento_totale_area_4->publish_state(val);
        }
        else if(i == 4)
        {
          inserimento_totale_area_5->publish_state(val);
        }
        else if(i == 5)
        {
          inserimento_totale_area_6->publish_state(val);
        }
        else if(i == 6)
        {
          inserimento_totale_area_7->publish_state(val);
        }
        else if(i == 7)
        {
          inserimento_totale_area_8->publish_state(val);
        }
      }

      // Ciclo AREE INSERITE PARZIALI
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[7] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 0)
        {
          inserimento_parziale_area_1->publish_state(val);
        }
        else if(i == 1)
        {
          inserimento_parziale_area_2->publish_state(val);
        }
        else if(i == 2)
        {
          inserimento_parziale_area_3->publish_state(val);
        }
        else if(i == 3)
        {
          inserimento_parziale_area_4->publish_state(val);
        }
        else if(i == 4)
        {
          inserimento_parziale_area_5->publish_state(val);
        }
        else if(i == 5)
        {
          inserimento_parziale_area_6->publish_state(val);
        }
        else if(i == 6)
        {
          inserimento_parziale_area_7->publish_state(val);
        }
        else if(i == 7)
        {
          inserimento_parziale_area_8->publish_state(val);
        }
        
      }


      // Ciclo AREE INSERITE PARZIALI RITARDO 0
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[8] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 0)
        {
          inserimento_parziale_ritardo_0_area_1->publish_state(val);
        }
        else if(i == 1)
        {
          inserimento_parziale_ritardo_0_area_2->publish_state(val);
        }
        else if(i == 2)
        {
          inserimento_parziale_ritardo_0_area_3->publish_state(val);
        }
        else if(i == 3)
        {
          inserimento_parziale_ritardo_0_area_4->publish_state(val);
        }
        else if(i == 4)
        {
          inserimento_parziale_ritardo_0_area_5->publish_state(val);
        }
        else if(i == 5)
        {
          inserimento_parziale_ritardo_0_area_6->publish_state(val);
        }
        else if(i == 6)
        {
          inserimento_parziale_ritardo_0_area_7->publish_state(val);
        }
        else if(i == 7)
        {
          inserimento_parziale_ritardo_0_area_8->publish_state(val);
        }
      }


      // Ciclo AREE DISINSERITE
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[9] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 0)
        {
          disinserita_area_1->publish_state(val);
        }
        else if(i == 1)
        {
          disinserita_area_2->publish_state(val);
        }
        else if(i == 2)
        {
          disinserita_area_3->publish_state(val);
        }
        else if(i == 3)
        {
          disinserita_area_4->publish_state(val);
        }
        else if(i == 4)
        {
          disinserita_area_5->publish_state(val);
        }
        else if(i == 5)
        {
          disinserita_area_6->publish_state(val);
        }
        else if(i == 6)
        {
          disinserita_area_7->publish_state(val);
        }
        else if(i == 7)
        {
          disinserita_area_8->publish_state(val);
        }
      }

      // Ciclo STATO SIRENA
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[10] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 0)
        {
          stato_sirena->publish_state(val);
        }
      }


      // CICLO STATO USCITE
      for (i = 0; i < 32; i++)
      {
        mask = 00000001;
        StatoZona = -1;

        if (i >= 8 && i <= 15)
          StatoZona = (Rx[11] >> (i - 8)) & 1;
        else if (i <= 7)
          StatoZona = (Rx[12] >> i) & 1;

        //ESP_LOGD("custom", "The value of sensor is: %i", StatoZona);
        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }

        switch (i)
        {
        case 0:
          stato_uscita_1->publish_state(val);
          break;
        case 1:
          stato_uscita_2->publish_state(val);
          break;
        case 2:
          stato_uscita_3->publish_state(val);
          break;
        case 3:
          stato_uscita_4->publish_state(val);
          break;
        case 4:
          stato_uscita_5->publish_state(val);
          break;
        case 5:
          stato_uscita_6->publish_state(val);
          break;
        case 6:
          stato_uscita_7->publish_state(val);
          break;
        case 7:
          stato_uscita_8->publish_state(val);
          break;
        case 8:
          stato_uscita_9->publish_state(val);
          break;
        case 9:
          stato_uscita_10->publish_state(val);
          break;
        case 10:
          stato_uscita_11->publish_state(val);
          break;
        case 11:
          stato_uscita_12->publish_state(val);
          break;
        case 12:
          stato_uscita_13->publish_state(val);
          break;
        case 13:
          stato_uscita_14->publish_state(val);
          break;
        case 14:
          stato_uscita_15->publish_state(val);
          break;
        case 15:
          stato_uscita_16->publish_state(val);
          break;
        }
      }


      // CICLO ZONE ESCLUSE
      for (i = 0; i < 32; i++)
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
        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }

        switch (i)
        {
        case 0:
          zona_esclusa_1->publish_state(val);
          break;
        case 1:
          zona_esclusa_2->publish_state(val);
          break;
        case 2:
          zona_esclusa_3->publish_state(val);
          break;
        case 3:
          zona_esclusa_4->publish_state(val);
          break;
        case 4:
          zona_esclusa_5->publish_state(val);
          break;
        case 5:
          zona_esclusa_6->publish_state(val);
          break;
        case 6:
          zona_esclusa_7->publish_state(val);
          break;
        case 7:
          zona_esclusa_8->publish_state(val);
          break;
        case 8:
          zona_esclusa_9->publish_state(val);
          break;
        case 9:
          zona_esclusa_10->publish_state(val);
          break;
        case 10:
          zona_esclusa_11->publish_state(val);
          break;
        case 11:
          zona_esclusa_12->publish_state(val);
          break;
        case 12:
          zona_esclusa_13->publish_state(val);
          break;
        case 13:
          zona_esclusa_14->publish_state(val);
          break;
        case 14:
          zona_esclusa_15->publish_state(val);
          break;
        case 15:
          zona_esclusa_16->publish_state(val);
          break;
        case 16:
          zona_esclusa_17->publish_state(val);
          break;
        case 17:
          zona_esclusa_18->publish_state(val);
          break;
        case 18:
          zona_esclusa_19->publish_state(val);
          break;
        case 19:
          zona_esclusa_20->publish_state(val);
          break;
        case 20:
          zona_esclusa_21->publish_state(val);
          break;
        case 21:
          zona_esclusa_22->publish_state(val);
          break;
        case 22:
          zona_esclusa_23->publish_state(val);
          break;
        case 23:
          zona_esclusa_24->publish_state(val);
          break;
        case 24:
          zona_esclusa_25->publish_state(val);
          break;
        case 25:
          zona_esclusa_26->publish_state(val);
          break;
        case 26:
          zona_esclusa_27->publish_state(val);
          break;
        case 27:
          zona_esclusa_28->publish_state(val);
          break;
        case 28:
          zona_esclusa_29->publish_state(val);
          break;
        case 29:
          zona_esclusa_30->publish_state(val);
          break;
        case 30:
          zona_esclusa_31->publish_state(val);
          break;
        case 31:
          zona_esclusa_32->publish_state(val);
          break;
        }
      }

      // CICLO MEMORIA ALLARME ZONE
      for (i = 0; i < 32; i++)
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
        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }

        switch (i)
        {
        case 0:
          memoria_allarme_zona_1->publish_state(val);
          break;
        case 1:
          memoria_allarme_zona_2->publish_state(val);
          break;
        case 2:
          memoria_allarme_zona_3->publish_state(val);
          break;
        case 3:
          memoria_allarme_zona_4->publish_state(val);
          break;
        case 4:
          memoria_allarme_zona_5->publish_state(val);
          break;
        case 5:
          memoria_allarme_zona_6->publish_state(val);
          break;
        case 6:
          memoria_allarme_zona_7->publish_state(val);
          break;
        case 7:
          memoria_allarme_zona_8->publish_state(val);
          break;
        case 8:
          memoria_allarme_zona_9->publish_state(val);
          break;
        case 9:
          memoria_allarme_zona_10->publish_state(val);
          break;
        case 10:
          memoria_allarme_zona_11->publish_state(val);
          break;
        case 11:
          memoria_allarme_zona_12->publish_state(val);
          break;
        case 12:
          memoria_allarme_zona_13->publish_state(val);
          break;
        case 13:
          memoria_allarme_zona_14->publish_state(val);
          break;
        case 14:
          memoria_allarme_zona_15->publish_state(val);
          break;
        case 15:
          memoria_allarme_zona_16->publish_state(val);
          break;
        case 16:
          memoria_allarme_zona_17->publish_state(val);
          break;
        case 17:
          memoria_allarme_zona_18->publish_state(val);
          break;
        case 18:
          memoria_allarme_zona_19->publish_state(val);
          break;
        case 19:
          memoria_allarme_zona_20->publish_state(val);
          break;
        case 20:
          memoria_allarme_zona_21->publish_state(val);
          break;
        case 21:
          memoria_allarme_zona_22->publish_state(val);
          break;
        case 22:
          memoria_allarme_zona_23->publish_state(val);
          break;
        case 23:
          memoria_allarme_zona_24->publish_state(val);
          break;
        case 24:
          memoria_allarme_zona_25->publish_state(val);
          break;
        case 25:
          memoria_allarme_zona_26->publish_state(val);
          break;
        case 26:
          memoria_allarme_zona_27->publish_state(val);
          break;
        case 27:
          memoria_allarme_zona_28->publish_state(val);
          break;
        case 28:
          memoria_allarme_zona_29->publish_state(val);
          break;
        case 29:
          memoria_allarme_zona_30->publish_state(val);
          break;
        case 30:
          memoria_allarme_zona_31->publish_state(val);
          break;
        case 31:
          memoria_allarme_zona_32->publish_state(val);
          break;
        }
      }


      // CICLO MEMORIA SABOTAGGIO ZONE
      for (i = 0; i < 32; i++)
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
        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }

        switch (i)
        {
        case 0:
          memoria_sabotaggio_zona_1->publish_state(val);
          break;
        case 1:
          memoria_sabotaggio_zona_2->publish_state(val);
          break;
        case 2:
          memoria_sabotaggio_zona_3->publish_state(val);
          break;
        case 3:
          memoria_sabotaggio_zona_4->publish_state(val);
          break;
        case 4:
          memoria_sabotaggio_zona_5->publish_state(val);
          break;
        case 5:
          memoria_sabotaggio_zona_6->publish_state(val);
          break;
        case 6:
          memoria_sabotaggio_zona_7->publish_state(val);
          break;
        case 7:
          memoria_sabotaggio_zona_8->publish_state(val);
          break;
        case 8:
          memoria_sabotaggio_zona_9->publish_state(val);
          break;
        case 9:
          memoria_sabotaggio_zona_10->publish_state(val);
          break;
        case 10:
          memoria_sabotaggio_zona_11->publish_state(val);
          break;
        case 11:
          memoria_sabotaggio_zona_12->publish_state(val);
          break;
        case 12:
          memoria_sabotaggio_zona_13->publish_state(val);
          break;
        case 13:
          memoria_sabotaggio_zona_14->publish_state(val);
          break;
        case 14:
          memoria_sabotaggio_zona_15->publish_state(val);
          break;
        case 15:
          memoria_sabotaggio_zona_16->publish_state(val);
          break;
        case 16:
          memoria_sabotaggio_zona_17->publish_state(val);
          break;
        case 17:
          memoria_sabotaggio_zona_18->publish_state(val);
          break;
        case 18:
          memoria_sabotaggio_zona_19->publish_state(val);
          break;
        case 19:
          memoria_sabotaggio_zona_20->publish_state(val);
          break;
        case 20:
          memoria_sabotaggio_zona_21->publish_state(val);
          break;
        case 21:
          memoria_sabotaggio_zona_22->publish_state(val);
          break;
        case 22:
          memoria_sabotaggio_zona_23->publish_state(val);
          break;
        case 23:
          memoria_sabotaggio_zona_24->publish_state(val);
          break;
        case 24:
          memoria_sabotaggio_zona_25->publish_state(val);
          break;
        case 25:
          memoria_sabotaggio_zona_26->publish_state(val);
          break;
        case 26:
          memoria_sabotaggio_zona_27->publish_state(val);
          break;
        case 27:
          memoria_sabotaggio_zona_28->publish_state(val);
          break;
        case 28:
          memoria_sabotaggio_zona_29->publish_state(val);
          break;
        case 29:
          memoria_sabotaggio_zona_30->publish_state(val);
          break;
        case 30:
          memoria_sabotaggio_zona_31->publish_state(val);
          break;
        case 31:
          memoria_sabotaggio_zona_32->publish_state(val);
          break;
        }
      }
    }
  }

  void update_sensor_status()
  {
    bool val;
    byte Rx[255];
    int Count = 0, i;

    int StatoZona;
    byte mask = 1;

    val = getParm(cmdGetSensorStatus, sizeof(cmdGetSensorStatus), Rx, Count);

    if (Count == 18)
    {
      // Ciclo ZONE
      for (i = 0; i < 32; i++)
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
        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }

        switch (i)
        {
        case 0:
          zona_1->publish_state(val);
          break;
        case 1:
          zona_2->publish_state(val);
          break;
        case 2:
          zona_3->publish_state(val);
          break;
        case 3:
          zona_4->publish_state(val);
          break;
        case 4:
          zona_5->publish_state(val);
          break;
        case 5:
          zona_6->publish_state(val);
          break;
        case 6:
          zona_7->publish_state(val);
          break;
        case 7:
          zona_8->publish_state(val);
          break;
        case 8:
          zona_9->publish_state(val);
          break;
        case 9:
          zona_10->publish_state(val);
          break;
        case 10:
          zona_11->publish_state(val);
          break;
        case 11:
          zona_12->publish_state(val);
          break;
        case 12:
          zona_13->publish_state(val);
          break;
        case 13:
          zona_14->publish_state(val);
          break;
        case 14:
          zona_15->publish_state(val);
          break;
        case 15:
          zona_16->publish_state(val);
          break;
        case 16:
          zona_17->publish_state(val);
          break;
        case 17:
          zona_18->publish_state(val);
          break;
        case 18:
          zona_19->publish_state(val);
          break;
        case 19:
          zona_20->publish_state(val);
          break;
        case 20:
          zona_21->publish_state(val);
          break;
        case 21:
          zona_22->publish_state(val);
          break;
        case 22:
          zona_23->publish_state(val);
          break;
        case 23:
          zona_24->publish_state(val);
          break;
        case 24:
          zona_25->publish_state(val);
          break;
        case 25:
          zona_26->publish_state(val);
          break;
        case 26:
          zona_27->publish_state(val);
          break;
        case 27:
          zona_28->publish_state(val);
          break;
        case 28:
          zona_29->publish_state(val);
          break;
        case 29:
          zona_30->publish_state(val);
          break;
        case 30:
          zona_31->publish_state(val);
          break;
        case 31:
          zona_32->publish_state(val);
          break;
        }
      }

      
      // Ciclo SABOTAGGIO ZONE
      for (i = 0; i < 32; i++)
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

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        switch (i)
        {
        case 0:
          zona_sabotaggio_1->publish_state(val);
          break;
        case 1:
          zona_sabotaggio_2->publish_state(val);
          break;
        case 2:
          zona_sabotaggio_3->publish_state(val);
          break;
        case 3:
          zona_sabotaggio_4->publish_state(val);
          break;
        case 4:
          zona_sabotaggio_5->publish_state(val);
          break;
        case 5:
          zona_sabotaggio_6->publish_state(val);
          break;
        case 6:
          zona_sabotaggio_7->publish_state(val);
          break;
        case 7:
          zona_sabotaggio_8->publish_state(val);
          break;
        case 8:
          zona_sabotaggio_9->publish_state(val);
          break;
        case 9:
          zona_sabotaggio_10->publish_state(val);
          break;
        case 10:
          zona_sabotaggio_11->publish_state(val);
          break;
        case 11:
          zona_sabotaggio_12->publish_state(val);
          break;
        case 12:
          zona_sabotaggio_13->publish_state(val);
          break;
        case 13:
          zona_sabotaggio_14->publish_state(val);
          break;
        case 14:
          zona_sabotaggio_15->publish_state(val);
          break;
        case 15:
          zona_sabotaggio_16->publish_state(val);
          break;
        case 16:
          zona_sabotaggio_17->publish_state(val);
          break;
        case 17:
          zona_sabotaggio_18->publish_state(val);
          break;
        case 18:
          zona_sabotaggio_19->publish_state(val);
          break;
        case 19:
          zona_sabotaggio_20->publish_state(val);
          break;
        case 20:
          zona_sabotaggio_21->publish_state(val);
          break;
        case 21:
          zona_sabotaggio_22->publish_state(val);
          break;
        case 22:
          zona_sabotaggio_23->publish_state(val);
          break;
        case 23:
          zona_sabotaggio_24->publish_state(val);
          break;
        case 24:
          zona_sabotaggio_25->publish_state(val);
          break;
        case 25:
          zona_sabotaggio_26->publish_state(val);
          break;
        case 26:
          zona_sabotaggio_27->publish_state(val);
          break;
        case 27:
          zona_sabotaggio_28->publish_state(val);
          break;
        case 28:
          zona_sabotaggio_29->publish_state(val);
          break;
        case 29:
          zona_sabotaggio_30->publish_state(val);
          break;
        case 30:
          zona_sabotaggio_31->publish_state(val);
          break;
        case 31:
          zona_sabotaggio_32->publish_state(val);
          break;
        }
      }


      // Ciclo WARNINGS
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[14] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 0)
        {
          warn_mancanza_rete->publish_state(val);
        }
        else if(i == 1)
        {
          warn_scomparsa_bpi->publish_state(val);
        }
        else if(i == 2)
        {
          warn_fusibile->publish_state(val);
        }
        else if(i == 3)
        {
          warn_batteria_bassa->publish_state(val);
        }
        else if(i == 4)
        {
          warn_guasto_linea_telefonica->publish_state(val);
        }
        else if(i == 5)
        {
          warn_codici_default->publish_state(val);
        }
        else if(i == 6)
        {
          warn_wireless->publish_state(val);
        }
        
      }


      // Ciclo ALLARME AREA
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[15] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        switch (i)
        {
        case 0:
          allarme_area_1->publish_state(val);
          break;
        case 1:
          allarme_area_2->publish_state(val);
          break;
        case 2:
          allarme_area_3->publish_state(val);
          break;
        case 3:
          allarme_area_4->publish_state(val);
          break;
        case 4:
          allarme_area_5->publish_state(val);
          break;
        case 5:
          allarme_area_6->publish_state(val);
          break;
        case 6:
          allarme_area_7->publish_state(val);
          break;
        case 7:
          allarme_area_8->publish_state(val);
          break;
        }
      }

      // Ciclo SABOTAGGI
      for (i = 0; i < 8; i++)
      {
        mask = 00000001;
        StatoZona = -1;
        StatoZona = (Rx[16] >> i) & 1;

        if (StatoZona == 1)
        {
          val = true;
        }
        else
        {
          val = false;
        }
        
        if(i == 2)
        {
          sabotaggio_zona->publish_state(val);
        }
        else if(i == 3)
        {
          sabotaggio_chiave_falsa->publish_state(val);
        }
        else if(i == 4)
        {
          sabotaggio_bpi->publish_state(val);
        }
        else if(i == 5)
        {
          sabotaggio_sistema->publish_state(val);
        }
        else if(i == 6)
        {
          sabotaggio_jam->publish_state(val);
        }
        else if(i == 7)
        {
          sabotaggio_wireless->publish_state(val);
        }
        
      }

    }
  }
};
