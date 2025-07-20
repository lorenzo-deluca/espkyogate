/*
* espkyogate - ESPHome custom component for Bentel KYO alarms
* Copyright (C) 2025 Lorenzo De Luca (me@lorenzodeluca.dev)
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
*
* GNU Affero General Public License v3.0
*/

#include <sstream>
#include "esphome.h"
#define byte uint8_t

#define KYO_MAX_ZONE 32
#define KYO_MAX_ZONE_8 8
#define KYO_MAX_AREE 8
#define KYO_MAX_USCITE 8

#define UPDATE_INT_MS 500

class Bentel_Kyo32 : public esphome::PollingComponent, public uart::UARTDevice, public api::CustomAPIDevice {
	public:
		Bentel_Kyo32(UARTComponent *parent) : UARTDevice(parent) {}
		
		BinarySensor *kyo_comunication = new BinarySensor();

		BinarySensor *warn_mancanza_rete = new BinarySensor();
		BinarySensor *warn_scomparsa_bpi = new BinarySensor();
		BinarySensor *warn_fusibile = new BinarySensor();
		BinarySensor *warn_batteria_bassa = new BinarySensor();
		BinarySensor *warn_guasto_linea_telefonica = new BinarySensor();
		BinarySensor *warn_codici_default = new BinarySensor();
		BinarySensor *warn_wireless = new BinarySensor();

		BinarySensor *stato_sirena = new BinarySensor();
		BinarySensor *sabotaggio_zona = new BinarySensor();
		BinarySensor *sabotaggio_chiave_falsa = new BinarySensor();
		BinarySensor *sabotaggio_bpi = new BinarySensor();
		BinarySensor *sabotaggio_sistema = new BinarySensor();
		BinarySensor *sabotaggio_jam = new BinarySensor();
		BinarySensor *sabotaggio_wireless = new BinarySensor();

		// Zone
		BinarySensor* zona = new BinarySensor[KYO_MAX_ZONE];
		BinarySensor* zona_sabotaggio = new BinarySensor[KYO_MAX_ZONE];
		BinarySensor* zona_esclusa = new BinarySensor[KYO_MAX_ZONE];
		BinarySensor* memoria_allarme_zona = new BinarySensor[KYO_MAX_ZONE];
		BinarySensor* memoria_sabotaggio_zona = new BinarySensor[KYO_MAX_ZONE];

		// Aree
		BinarySensor* allarme_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* inserimento_totale_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* inserimento_parziale_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* inserimento_parziale_ritardo_0_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* disinserita_area = new BinarySensor[KYO_MAX_AREE];

		BinarySensor* stato_uscita = new BinarySensor[KYO_MAX_USCITE];

		void setup() override
		{
 			set_update_interval(UPDATE_INT_MS);
            set_setup_priority(setup_priority::AFTER_CONNECTION);

			register_service(&Bentel_Kyo32::arm_area, "arm_area", {"area", "arm_type", "specific_area"});
			register_service(&Bentel_Kyo32::disarm_area, "disarm_area", {"area", "specific_area"});
			register_service(&Bentel_Kyo32::reset_alarms, "reset_alarms");
			register_service(&Bentel_Kyo32::activate_output, "activate_output", {"output_number"});
			register_service(&Bentel_Kyo32::deactivate_output, "deactivate_output", {"output_number"});
			register_service(&Bentel_Kyo32::pulse_output, "pulse_output", {"output_number", "pulse_time"});
			register_service(&Bentel_Kyo32::debug_command, "debug_command", {"serial_trace", "log_trace", "polling_kyo"});
			register_service(&Bentel_Kyo32::update_datetime, "update_datetime", {"day", "month", "year", "hours", "minutes", "seconds"});

			register_service(&Bentel_Kyo32::include_zone, "include_zone", {"zone_number"});
			register_service(&Bentel_Kyo32::exclude_zone, "exclude_zone", {"zone_number"});

			pollingState = PollingStateEnum::Init;
			kyo_comunication->publish_state(false);
		}

		// ========================================= 
		// START COMMANDS 
		
		void arm_area(float area, float arm_type, float specific_area){
			return _int_arm_area((int)area, (int)arm_type, (int)specific_area);
		}

		void _int_arm_area(int area, int arm_type, int specific_area)
		{
			if (area > KYO_MAX_AREE)
			{
				ESP_LOGE("arm_area", "Invalid Area %i, MAX %i", area, KYO_MAX_AREE);
				return;
			}
			
			ESP_LOGI("arm_area", "request arm type %d area %d specific %d", arm_type, area, specific_area);
			byte cmdArmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0xFF};

			byte total_insert_area_status = 0x00, partial_insert_area_status = 0x00;

			if (specific_area == 1)
			{
				for (int i = 0; i < KYO_MAX_AREE; i++)
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
			int Count = sendMessageToKyo(cmdArmPartition, sizeof(cmdArmPartition), Rx, 250);
			ESP_LOGD("arm_area", "arm_area kyo respond %i", Count);
		}

		void disarm_area(float area, float specific_area){
			return _int_disarm_area((int)area, (int)specific_area);
		}	
		void _int_disarm_area(int area, int specific_area){
			if (area > KYO_MAX_AREE)
			{
				ESP_LOGE("disarm_area", "invalid area %i, MAX %i", area, KYO_MAX_AREE);
				return;
			}
			
			ESP_LOGI("disarm_area", "request disarm area %d , specific %d", area, specific_area);
			byte cmdDisarmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};

			byte total_insert_area_status = 0x00, partial_insert_area_status = 0x00;

			if (specific_area == 1)
			{
				for (int i = 0; i < KYO_MAX_AREE; i++)
				{
					total_insert_area_status |= (this->inserimento_totale_area[i].state) << i;
					partial_insert_area_status |= (this->inserimento_parziale_area[i].state) << i;
				}

				if (this->inserimento_totale_area[area - 1].state)
					total_insert_area_status &= ~(1 << (area - 1));
				else
					partial_insert_area_status &= ~(1 << (area - 1));
			}

			cmdDisarmPartition[6] = total_insert_area_status;
			cmdDisarmPartition[7] = partial_insert_area_status;
			cmdDisarmPartition[9] = calculateCRC(cmdDisarmPartition, 8);

			byte Rx[255];
			int Count = sendMessageToKyo(cmdDisarmPartition, sizeof(cmdDisarmPartition), Rx, 100);
			ESP_LOGD("disarm_area", "kyo respond %i", Count);
		}

		void reset_alarms()
		{
			ESP_LOGI("reset_alarms", "Reset Alarms.");

			byte Rx[255];
			int Count = sendMessageToKyo(cmdResetAllarms, sizeof(cmdResetAllarms), Rx, 250);
			ESP_LOGE("reset_alarms", "kyo respond %i", Count);
		}

		void debug_command(float serial_trace, float log_trace, float polling_kyo){
			return _int_debug_command((int)serial_trace, (int)log_trace, (int)polling_kyo);
		}
			
		void _int_debug_command(int serial_trace, int log_trace, int polling_kyo)
		{
			this->serialTrace = (serial_trace == 1);
			this->logTrace = (log_trace == 1);
			this->polling_kyo = (polling_kyo == 1);

			ESP_LOGI("debug_command", "serial_trace %i log_trace %i polling_kyo %i ", this->serialTrace, this->logTrace, this->polling_kyo);
		}

		void activate_output(float output_number) {
			return _int_activate_output((int)output_number);
		}
		
		void _int_activate_output(int output_number)
		{
			if (output_number > KYO_MAX_USCITE)
			{
				ESP_LOGE("activate_output", "invalid output %i, MAX %i", output_number, KYO_MAX_USCITE);
				return;
			}

			ESP_LOGI("activate_output", "activate Output Number: %d", output_number);
			
			byte cmdActivateOutput[9] = {0x0f, 0x06, 0xf0, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00};
			
			cmdActivateOutput[6] |= 1 << (output_number - 1);
			cmdActivateOutput[8] = cmdActivateOutput[6];

			byte Rx[255];
			int Count = sendMessageToKyo(cmdActivateOutput, sizeof(cmdActivateOutput), Rx, 250);
			ESP_LOGD("activate_output", "kyo respond %i", Count);
		}

		void deactivate_output(float output_number) {
			return _int_deactivate_output((int)output_number);
		}
		void _int_deactivate_output(int output_number)
		{
			if (output_number > KYO_MAX_USCITE)
			{
				ESP_LOGE("deactivate_output", "invalid output %i, MAX %i", output_number, KYO_MAX_USCITE);
				return;
			}

			ESP_LOGI("deactivate_output", "deactivate Output Number: %d", output_number);
			
			byte cmdDeactivateOutput[9] = {0x0f, 0x06, 0xf0, 0x01, 0x00, 0x06, 0x00, 0x00, 0xCC};
			
			cmdDeactivateOutput[7] |= 1 << (output_number - 1);
			cmdDeactivateOutput[8] = cmdDeactivateOutput[7];

			byte Rx[255];
			int Count = sendMessageToKyo(cmdDeactivateOutput, sizeof(cmdDeactivateOutput), Rx, 250);
			ESP_LOGD("deactivate_output", "kyo respond %i", Count);
		}

		void pulse_output(float output_number, float pulse_time){
			return _int_pulse_output((int)output_number, (int)pulse_time);
		}
		void _int_pulse_output(int output_number, int pulse_time)
		{
			if (output_number > KYO_MAX_USCITE)
			{
				ESP_LOGE("pulse_output", "invalid output %i, MAX %i", output_number, KYO_MAX_USCITE);
				return;
			}
			ESP_LOGI("pulse_output", "pulse Output Number: %d for pulse_time", output_number, pulse_time);

			activate_output(output_number);
			delay(pulse_time);
			deactivate_output(output_number);
			
			ESP_LOGD("pulse_output", "end");
		}
		
		void update_datetime(float day, float month, float year, float hours, float minutes, float seconds){
			return _int_update_datetime((int)day, (int)month, (int)year, (int)hours, (int)minutes, (int)seconds);
		}
		void _int_update_datetime(int day, int month, int year, int hours, int minutes, int seconds)
		{
			if (day <= 0 || day > 31 || month <= 0 || month > 12 || year < 2000 || year > 2099 ||
				minutes < 0 || minutes > 59 || seconds < 0 ||  seconds > 59)
			{
				ESP_LOGE("update_datetime", "invalid datetime");
				return;
			}

			ESP_LOGI("update_datetime", "recive %d/%d/%d %d:%d:%d",  day, month, year, hours, minutes, seconds);
		
			byte cmdUpdateDateTime[13] = {0x0f, 0x03, 0xf0, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			
			cmdUpdateDateTime[6] = day;
			cmdUpdateDateTime[7] = month;
			cmdUpdateDateTime[8] = year-2000;
			cmdUpdateDateTime[9] = hours;
			cmdUpdateDateTime[10] = minutes;
			cmdUpdateDateTime[11] = seconds;
			cmdUpdateDateTime[12] = getChecksum(cmdUpdateDateTime, 6, 12);

			byte Rx[255];
			int Count = sendMessageToKyo(cmdUpdateDateTime, sizeof(cmdUpdateDateTime), Rx, 300);
			ESP_LOGD("update_datetime", "kyo respond %i", Count);
		}

		void include_zone(float zone_number) {
			return _int_include_zone((int)zone_number);
		}
		void _int_include_zone(int zone_number)
		{
			if (zone_number > KYO_MAX_ZONE)
			{
				ESP_LOGE("include_zone", "invalid zone %i, MAX %i", zone_number, KYO_MAX_ZONE);
				return;
			}

			ESP_LOGI("include_zone", "request Include Zone  Number: %d", zone_number);
			
			// 0f 01 f0 07 00 07 00 00 00 00 00 00 00 01 01
			byte cmdIncludeZone[15] = {0x0f, 0x01, 0xf0, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			
			if (zone_number > 24){
				cmdIncludeZone[10] |= 1 << (zone_number - 25);
				cmdIncludeZone[14] = cmdIncludeZone[10];
			}
			else if (zone_number > 16 && zone_number <= 24){
				cmdIncludeZone[11] |= 1 << (zone_number - 17);	
				cmdIncludeZone[14] = cmdIncludeZone[11];
			}
			else if (zone_number > 8 && zone_number <= 16){
				cmdIncludeZone[12] |= 1 << (zone_number - 9);
				cmdIncludeZone[14] = cmdIncludeZone[12];
			}
			else if (zone_number <= 8){
				cmdIncludeZone[13] |= 1 << (zone_number - 1);
				cmdIncludeZone[14] = cmdIncludeZone[13];
			}

			byte Rx[255];
			int Count = sendMessageToKyo(cmdIncludeZone, sizeof(cmdIncludeZone), Rx, 250);
			ESP_LOGD("include_zone", "kyo respond %i", Count);
		}

		void exclude_zone(float zone_number) {
			return _int_exclude_zone((int)zone_number);
		}
		void _int_exclude_zone(int zone_number)
		{
			if (zone_number > KYO_MAX_ZONE)
			{
				ESP_LOGE("exclude_zone", "invalid zone %i, MAX %i", zone_number, KYO_MAX_ZONE);
				return;
			}

			ESP_LOGI("exclude_zone", "request Exclude Zone  Number: %d", zone_number);
			
			// 0f 01 f0 07 00 07 00 00 00 00 00 00 00 01 01
			byte cmdExcludeZone[15] = {0x0f, 0x01, 0xf0, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			
			if (zone_number > 24){
				cmdExcludeZone[6] |= 1 << (zone_number - 25);
				cmdExcludeZone[14] = cmdExcludeZone[6];
			}
			else if (zone_number > 16 && zone_number <= 24){
				cmdExcludeZone[7] |= 1 << (zone_number - 17);	
				cmdExcludeZone[14] = cmdExcludeZone[7];
			}
			else if (zone_number > 8 && zone_number <= 16){
				cmdExcludeZone[8] |= 1 << (zone_number - 9);
				cmdExcludeZone[14] = cmdExcludeZone[8];
			}
			else if (zone_number <= 8){
				cmdExcludeZone[9] |= 1 << (zone_number - 1);
				cmdExcludeZone[14] = cmdExcludeZone[9];
			}

			byte Rx[255];
			int Count = sendMessageToKyo(cmdExcludeZone, sizeof(cmdExcludeZone), Rx, 250);
			ESP_LOGD("exclude_zone", "kyo respond %i", Count);
		}

		// END COMMANDS 
		// ========================================= 
		
		void update() override
		{
			if (!this->polling_kyo)
				return;

			switch(this->pollingState)
			{
				case PollingStateEnum::Init:
					if (this->update_kyo_status())
					{
						this->pollingState = PollingStateEnum::Status;
						this->centralInvalidMessageCount = 0;
					}
					else
						this->centralInvalidMessageCount++;

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

	private:

		byte cmdGetSensorStatus[6] = {0xf0, 0x04, 0xf0, 0x0a, 0x00, 0xee};	  // Read Realtime Status and Trouble Status
		byte cmdGetPartitionStatus[6] = {0xf0, 0x02, 0x15, 0x12, 0x00, 0x19}; // Partitions Status (305) - Outputs Status - Tamper Memory - Bypassed Zones - Zone Alarm Memory - Zone Tamper Memory
		byte cmdGetPartitionStatus_Kyo8[6] = {0xf0, 0x68, 0x0e, 0x09, 0x00, 0x6f}; // Partitions Status (305) - Outputs Status - Tamper Memory - Bypassed Zones - Zone Alarm Memory - Zone Tamper Memory
		byte cmqGetSoftwareVersion[6] = {0xf0, 0x00, 0x00, 0x0b, 0x00, 0xfb}; // f0 00 00 0b 00 fb
		byte cmdResetAllarms[9] = {0x0F, 0x05, 0xF0, 0x01, 0x00, 0x05, 0xff, 0x00, 0xff};

		const std::vector<uint8_t> cmdGetAlarmInfo = {0xf0, 0x00, 0x00, 0x0b, 0x00};
        const size_t RPL_GET_ALARM_INFO_SIZE = 13;

		enum class AlarmModel {UNKNOWN, KYO_4, KYO_8, KYO_8G, KYO_32, KYO_32G, KYO_8W, KYO_8GW};
        AlarmModel alarmModel = AlarmModel::UNKNOWN;

        enum class AlarmStatus {UNAVAILABLE, PENDING, ARMING, ARMED_AWAY, ARMED_HOME, DISARMED, TRIGGERED};
        AlarmStatus alarmStatus = AlarmStatus::UNAVAILABLE;

		enum class PollingStateEnum { Init = 1, Status };
		PollingStateEnum pollingState;

		enum class PartitionStatusEnum { Idle = 1, Exclude, MemAlarm, MemSabotate};
		PartitionStatusEnum PartitionStatusInternal[KYO_MAX_ZONE];

		bool serialTrace = false;
		bool logTrace = false;
		bool polling_kyo = true;
		int centralInvalidMessageCount = 0;
		int MaxZone = KYO_MAX_ZONE;

		bool update_kyo_partitions()
		{
			byte Rx[255];
			int Count = 0;

			if (alarmModel == AlarmModel::KYO_8)
				Count = sendMessageToKyo(cmdGetPartitionStatus_Kyo8, sizeof(cmdGetPartitionStatus_Kyo8), Rx, 100);
			else
				Count = sendMessageToKyo(cmdGetPartitionStatus, sizeof(cmdGetPartitionStatus), Rx, 100);

			if (Count != 26 && Count != 17)
			{
				if (this->logTrace)
					ESP_LOGE("update_kyo_partitions", "invalid message length %i", Count);

				return (false);
			}

			int StatoZona, i;
			//for(i = 0; i < KYO_MAX_ZONE; i++)
			//	this->PartitionStatusInternal[i] = PartitionStatusEnum::Idle;

			// Ciclo AREE INSERITE
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[6] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != inserimento_totale_area[i].state)
					ESP_LOGI("aree_totale", "Area %i - Stato %i", i, StatoZona);
				
				inserimento_totale_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo AREE INSERITE PARZIALI
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[7] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != inserimento_parziale_area[i].state)
					ESP_LOGI("aree_parziale", "Area %i - Stato %i", i, StatoZona);

				inserimento_parziale_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo AREE INSERITE PARZIALI RITARDO 0
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[8] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != inserimento_parziale_ritardo_0_area[i].state)
					ESP_LOGI("inserimento_parziale_ritardo_0_area", "Area %i - Stato %i", i, StatoZona);

				inserimento_parziale_ritardo_0_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo AREE DISINSERITE
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[9] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != disinserita_area[i].state)
					ESP_LOGI("disinserita_area", "Area %i - Stato %i", i, StatoZona);

				disinserita_area[i].publish_state(StatoZona == 1);
			}

			// STATO SIRENA
			StatoZona = ((Rx[10] >> 5) & 1);
			if (this->logTrace && (StatoZona == 1) != stato_sirena->state)
				ESP_LOGI("stato_sirena", "Stato %i", StatoZona);
			stato_sirena->publish_state(StatoZona == 1);
			
			if (alarmModel == AlarmModel::KYO_32G)
			{
				// CICLO STATO USCITE
				for (i = 0; i < KYO_MAX_USCITE; i++)
				{
					StatoZona = (Rx[12] >> i) & 1;
					if (this->logTrace && (StatoZona == 1) != stato_uscita[i].state)
						ESP_LOGI("stato_uscita", "Uscita %i - Stato %i", i, StatoZona);

					stato_uscita[i].publish_state(StatoZona == 1);
				}
			}

			// CICLO ZONE ESCLUSE
			for (i = 0; i < MaxZone; i++)
			{
				StatoZona = 0;
				if (alarmModel == AlarmModel::KYO_8)
				{
					StatoZona = (Rx[11] >> i) & 1;	
				}
				else
				{
					if (i >= 24)
						StatoZona = (Rx[13] >> (i - 24)) & 1;
					else if (i >= 16 && i <= 23)
						StatoZona = (Rx[14] >> (i - 16)) & 1;
					else if (i >= 8 && i <= 15)
						StatoZona = (Rx[15] >> (i - 8)) & 1;
					else if (i <= 7)
						StatoZona = (Rx[16] >> i) & 1;	
				}
				
				zona_esclusa[i].publish_state(StatoZona == 1);
				if (StatoZona == 1)
					this->PartitionStatusInternal[i] = PartitionStatusEnum::Exclude;
			}

			// CICLO MEMORIA ALLARME ZONE
			for (i = 0; i < MaxZone; i++)
			{
				StatoZona = 0;
				if (alarmModel == AlarmModel::KYO_8)
				{
					StatoZona = (Rx[12] >> i) & 1;	
				}
				else
				{
					if (i >= 24)
						StatoZona = (Rx[17] >> (i - 24)) & 1;
					else if (i >= 16 && i <= 23)
						StatoZona = (Rx[18] >> (i - 16)) & 1;
					else if (i >= 8 && i <= 15)
						StatoZona = (Rx[19] >> (i - 8)) & 1;
					else if (i <= 7)
						StatoZona = (Rx[20] >> i) & 1;
				}
				
				memoria_allarme_zona[i].publish_state(StatoZona == 1);
				if (StatoZona == 1)
					this->PartitionStatusInternal[i] = PartitionStatusEnum::MemAlarm;
			}

			// CICLO MEMORIA SABOTAGGIO ZONE
			for (i = 0; i < MaxZone; i++)
			{
				StatoZona = 0;
				
				if (alarmModel == AlarmModel::KYO_8)
				{
					StatoZona = (Rx[13] >> i) & 1;	
				}
				else
				{
					if (i >= 24)
						StatoZona = (Rx[21] >> (i - 24)) & 1;
					else if (i >= 16 && i <= 23)
						StatoZona = (Rx[22] >> (i - 16)) & 1;
					else if (i >= 8 && i <= 15)
						StatoZona = (Rx[23] >> (i - 8)) & 1;
					else if (i <= 7)
						StatoZona = (Rx[24] >> i) & 1;
				}

				memoria_sabotaggio_zona[i].publish_state(StatoZona == 1);
				if (StatoZona == 1)
					this->PartitionStatusInternal[i] = PartitionStatusEnum::MemSabotate;
			}
			
			return true;
		}

		bool update_kyo_status()
		{
			byte Rx[255];
			int Count = 0;

			Count = sendMessageToKyo(cmdGetSensorStatus, sizeof(cmdGetSensorStatus), Rx, 100);
			switch(Count)
			{
				case 18: // Kyo 32G (default)
					MaxZone = KYO_MAX_ZONE;
					alarmModel = AlarmModel::KYO_32G;
					break;
				case 12: // Kyo 8 and Kyo 4
					MaxZone = KYO_MAX_ZONE_8;
					alarmModel = AlarmModel::KYO_8;
					break;
					
			default:
				if (this->logTrace)
					ESP_LOGE("update_kyo_status", "invalid message length %i", Count);
				
				return false;
			}

			int StatoZona, i;

			// Ciclo ZONE
			for (i = 0; i < MaxZone; i++)
			{
				StatoZona = 0;
				if (alarmModel == AlarmModel::KYO_8)
                {
                    StatoZona = (Rx[6] >> i) & 1;
                }
                else
                {
					if (i >= 24)
						StatoZona = (Rx[6] >> (i - 24)) & 1;
					else if (i >= 16 && i <= 23)
						StatoZona = (Rx[7] >> (i - 16)) & 1;
					else if (i >= 8 && i <= 15)
						StatoZona = (Rx[8] >> (i - 8)) & 1;
					else if (i <= 7)
						StatoZona = (Rx[9] >> i) & 1;
				}

				if (this->logTrace && (StatoZona == 1) != zona[i].state)	
					ESP_LOGI("stato_zona", "Zona %i - Stato %i", i, StatoZona);

				zona[i].publish_state(StatoZona==1);
			}

			// Ciclo SABOTAGGIO ZONE
			for (i = 0; i < MaxZone; i++)
			{
				StatoZona = 0;
				if (alarmModel == AlarmModel::KYO_8)
                {
                    StatoZona = (Rx[7] >> i) & 1;
                }
                else
                {
					if (i >= 24)
						StatoZona = (Rx[10] >> (i - 24)) & 1;
					else if (i >= 16 && i <= 23)
						StatoZona = (Rx[11] >> (i - 16)) & 1;
					else if (i >= 8 && i <= 15)
						StatoZona = (Rx[12] >> (i - 8)) & 1;
					else if (i <= 7)
						StatoZona = (Rx[13] >> i) & 1;
				}

				zona_sabotaggio[i].publish_state(StatoZona == 1);
			}

			// Ciclo ALLARME AREA
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				if (alarmModel == AlarmModel::KYO_8)
                    StatoZona = (Rx[9] >> i) & 1;
                else
					StatoZona = (Rx[15] >> i) & 1;

				allarme_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo WARNINGS
			for (i = 0; i < 8; i++)
			{
				if (alarmModel == AlarmModel::KYO_8) 
				{
					StatoZona = (Rx[8] >> i) & 1;
					// ciclo su bit di warning
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

						case 5:
							warn_guasto_linea_telefonica->publish_state(StatoZona == 1);
							break;

						case 6:
							warn_codici_default->publish_state(StatoZona == 1);
							break;
					}
				}
				else
				{
					StatoZona = (Rx[14] >> i) & 1;

					// ciclo su bit di warning
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
			}

			// Ciclo SABOTAGGI
			for (i = 0; i < 8; i++)
			{
				if (alarmModel == AlarmModel::KYO_8)
				{
					StatoZona = (Rx[10] >> i) & 1;
					switch(i)
					{
						case 4:
							sabotaggio_zona->publish_state(StatoZona == 1);
							break;

						case 5:
							sabotaggio_chiave_falsa->publish_state(StatoZona == 1);
							break;

						case 6:
							sabotaggio_bpi->publish_state(StatoZona == 1);
							break;

						case 7:
							sabotaggio_sistema->publish_state(StatoZona == 1);
							break;
					}
				}
				else
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
			}

			return true;
		}
		
		int sendMessageToKyo(byte *cmd, int lcmd, byte ReadByes[], int waitForAnswer = 100)
		{
			// clean rx buffer
			while (available() > 0)
				read();

			int index = 0;
			byte RxBuff[255];
			memset(ReadByes, 0, 254);

			write_array(cmd, lcmd);
			delay(waitForAnswer);

			// Read a single Byte
			while (available() > 0)
				RxBuff[index++] = read();

			if (this->serialTrace || waitForAnswer > 100)
				ESP_LOGI("sendMessageToKyo", "TX '%s'", format_hex_pretty(cmd, lcmd).c_str());

			if (index <= 0)
			{
				ESP_LOGE("sendMessageToKyo", "no answer from serial port");
				return -1;
			}
				
			if (this->serialTrace || waitForAnswer > 100)
				ESP_LOGI("sendMessageToKyo", "RX '%s'", format_hex_pretty(RxBuff, index).c_str());
			
			memcpy(ReadByes, RxBuff, index);
			return index;
		}

		byte calculateCRC(byte *cmd, int lcmd)
		{
			int sum = 0x00;
			for (int i = 0; i <= lcmd; i++)
				sum += cmd[i];

			return (0x203 - sum);
		}

		uint8_t getChecksum(byte *data, int offset, int len) {
            uint8_t ckSum = 0x00;
            for (int i = offset; i < len; i++)
                ckSum += data[i];

            return (ckSum);
        }
};
