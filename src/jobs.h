#pragma once

extern uint8_t jobs_count;

void jobs_list_save(uint8_t first_key);
void jobs_list_load(uint8_t first_key, const uint8_t version);
void jobs_list_move_to_top(uint8_t index);
void jobs_list_write_dict(DictionaryIterator *iter, uint8_t first_key);
void jobs_list_read_dict(DictionaryIterator *iter, uint8_t first_key, const uint8_t version);

void jobs_delete_all_jobs(void);
void jobs_delete_job_and_save(uint8_t index);
void jobs_add_job();
void jobs_rename_job(uint8_t index);
uint32_t jobs_get_job_seconds(uint8_t index);
void jobs_set_job_seconds(uint8_t index, time_t seconds);
char* jobs_get_job_name(uint8_t index);
char* jobs_get_job_clock_as_text(uint8_t index, bool days);
void jobs_stop_timer(void);
void jobs_reset_and_save(uint8_t index);