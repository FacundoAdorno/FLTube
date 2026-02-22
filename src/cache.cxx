/*
 * Copyright (C) 2025-2026 - FLtube
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#include "../include/cache.h"

std::string CacheEntry::EMPTY_VALUE = "";

CacheEntry::CacheEntry(std::string value, unsigned int TTL, unsigned long int custom_creation_date): value(value) {
    ttl = (TTL > 0) ? TTL : DEFAULT_ENTRY_TTL;
    if (custom_creation_date > 0) {
        creation_date = custom_creation_date;
    } else {
        creation_date = time(0);    // Set current datetime.
    }
    std::string check_value = value;
    // Check if value entry is valid...
    trim(check_value);
    if (check_value.empty())
        mark_invalid();
    else
        still_valid = true;
};

bool CacheEntry::is_valid() {
    // Test if this entry was not marked as invalid previously...'
    if (!this->still_valid) return false;
    time_t current_t = time(0);
    if (current_t >= creation_date && current_t < creation_date + ttl) {
        return true;
    } else {
        mark_invalid();
        return false;
    }
}

std::string CacheEntry::get(){
    if (this->is_valid()){
        return this->value;
    }
    // otherwise...
    return CacheEntry::EMPTY_VALUE;
}

time_t CacheEntry::get_expiration() {
    return this->creation_date + ttl;
}

void GeneralCache::init() {
    if (cache_entry_ttl < 120) {
        this->logger->warn(_("The time-to-live for a cache entry must not be less than 120 seconds. Setting to default value."));
        cache_entry_ttl = CacheEntry::DEFAULT_ENTRY_TTL;
    }
    this->logger->debug("Application cache was created.");
    load();
}

CacheEntry* GeneralCache::search(std::string id) {
    auto position = entries->find(id);
    if (position != entries->end()) {
        return position->second;
    }
    return nullptr;
}

void GeneralCache::add_entry(std::string id, const std::string value) {
    // Sanitize the entry value...
    std::string sanit_value = value;
    trim(sanit_value);
    if (sanit_value.empty()) {
        char mssg[256];
        snprintf(mssg, sizeof(mssg), _("[ID = %s] Cannot create a cache entry whose value is empty. Aborting operation..."), id.c_str());
        logger->warn(mssg);
        return;
    }
    CacheEntry* ce = this->search(id);
    if (ce != nullptr) {
        if (!ce->is_valid() || ce->get() != sanit_value) {
            // Delete old entry if is invalid or new value is different from saved.
            if ( this->remove_entry(id) )
                logger->debug(_("Cache entry DELETED - ID = ") + id);
        } else {
            // Otherwise, finalize the operation without adding or updating any cache entry...
            return;
        }
    }
    // Add the new or updated cache entry.
    entries->insert(
        std::make_pair(id, new CacheEntry(sanit_value, this->cache_entry_ttl))
    );
    logger->debug(_("Creating a new cache entry for ID = ") + id);
}

bool GeneralCache::remove_entry(std::string id) {
    auto position = entries->find(id);
    if (position != entries->end()) {
        CacheEntry* ce = position->second;
        entries->erase(position);
        delete ce;
    }
    // Returns true if the element was removed from map.
    return (position != entries->end());
}

std::string GeneralCache::get_entry_value(std::string id) {
    CacheEntry* ce = this->search(id);
    if (ce != nullptr && ce->is_valid()) {
        // logger->debug("FOUND A VALUE CACHED FOR ID = " + id + ". THIS IS THE VALUE: " + ce->get());
        return ce->get();
    }
    return CacheEntry::EMPTY_VALUE;
}

GeneralCache::~GeneralCache() {
    for (auto& pair : *entries) {
        delete pair.second;
    }
    this->entries->clear();
}

void GeneralCache::finish() {
    this->save();
}

bool GeneralCache::is_cached(std::string id) {
    auto position = entries->find(id);
    return ( position != entries->end() && position->second->is_valid());
}

std::string GeneralCache::get_cache_expiration_date(std::string id) {
    std::string result = "UNKNOWN";
    CacheEntry* ce = search(id);
    if (ce != nullptr) {
        char time_buffer[128];
        time_t t = ce->get_expiration();
        std::strftime(time_buffer, sizeof(time_buffer), "%X", localtime(&t));
        result = time_buffer;
    }
    return result;
}

int GeneralCache::load(){
    logger->warn(_("This is a 'in memory' cache, so no load data function is available. Ignoring this call..."));
    return 0;
}

int GeneralCache::save() {
    logger->warn(_("This is a 'in memory' cache, so no save function is available. Ignoring this call..."));
    return 0;
}

bool PermanentDiskCache::validate_save_directory() {
    char err_msg[256];
    if (dirname.empty()) {
        logger->error(_("No cache directory was specified. Please, fix it."));
        return false;
    } else if (!std::filesystem::exists(dirname) || !std::filesystem::is_directory(dirname)) {
        snprintf(err_msg, sizeof(err_msg), _("The cache directory %s is invalid. Please, create it."), dirname.c_str());
        logger->error(err_msg);
        return false;
    }
    return true;
}

bool PermanentDiskCache::validate_save_filename() {
    if (filename.empty()) {
        logger->error(_("The cache filename was not specified."));
        return false;
    }
    return true;
}

void PermanentDiskCache::set_save_directory_path(std::string dirname_path, std::string filename) {
    trim(dirname_path);
    this->dirname = dirname_path;
    this->filename = filename;
    if (!validate_save_directory()) {
        logger->info(_("Cache directory falls back to a temporary one: ") + DEFAULT_SAVE_PATH);
        this->dirname = DEFAULT_SAVE_PATH;
    }
    if (!validate_save_filename()) {
        this->filename = DEFAULT_SAVE_FILENAME;
    }
}

int PermanentDiskCache::load() {
    char message_bffr[1024];
    std::ifstream cache_file;
    if (dirname.empty() || filename.empty()) {
        logger->error(_("Aborting cache data load because no cache path was specified..."));
        return 1;
    }
    std::string cache_file_path = dirname + "/" + filename;
    cache_file.open(cache_file_path, std::ofstream::in);
    if ( ! cache_file.is_open()) {
        snprintf(message_bffr, sizeof(message_bffr), _("No cache file found at %s. Proceeding without loading cache data..."), cache_file_path.c_str());
        logger->warn(message_bffr);
        return 1;
    }
    // If cache file exists and is open, proceed to read every line to populate initial cache.
    std::string line, id, final_url;
    std::vector<std::string> fields;
    int count_rejected = 0, count_not_valid = 0, total_count_lines = 0, ttl;
    unsigned long int creation_date_time;
    while (std::getline(cache_file, line)) {
        total_count_lines++;
        trim(line);
        fields = tokenize(line, PermanentDiskCache::FIELD_SEPARATOR);
        if (fields.size() != PermanentDiskCache::TOTAL_FIELDS_SAVED ) {
            count_rejected++;
            continue;
        }
        id = fields.at(FIELDS_POS::ID);
        final_url = fields.at(FIELDS_POS::FINAL_URL);
        trim(id); trim(final_url);
        if (id.empty() || final_url.empty() || is_cached(id)) {
            count_rejected++;
            continue;
        }
        if (!isNumber(fields.at(FIELDS_POS::CREATION_DATE)) || !isNumber(fields.at(FIELDS_POS::TTL))) {
            count_rejected++;
            continue;
        }
        try {
            creation_date_time = std::stoi(fields.at(FIELDS_POS::CREATION_DATE));
            ttl = std::stoi(fields.at(FIELDS_POS::TTL));
        } catch (const std::invalid_argument& e) {
            count_rejected++;
            continue;
        }
        time_t current_time = time(0);
        if (creation_date_time + ttl <current_time) {
            // The cache entry become invalid due to expiration by date...
            count_not_valid++;
            continue;
        }
        entries->insert(
            std::make_pair(id, new CacheEntry(final_url, ttl, creation_date_time))
        );
    }
    snprintf(message_bffr, sizeof(message_bffr), _("Cache load statistics: ENTRIES_VALID=%d; ENTRIES_REJECTED=%d; ENTRIES_INVALID=%d; TOTAL_ENTRIES_PROCESSED=%d."), (total_count_lines - count_rejected - count_not_valid) ,count_rejected, count_not_valid, total_count_lines);
    logger->debug(message_bffr);
    return 0;
}

int PermanentDiskCache::save() {
    if (validate_save_directory() && validate_save_filename()) {
        std::ofstream outputfile;
        //Open file (and create if not exists) for write.
        outputfile.open(dirname + "/" + filename, std::ofstream::trunc);
        for ( auto it = entries->begin(); it != entries->end(); it++ ) {
            // saving every cache entry data as a line of the outputfile.
            CacheEntry* ce = it->second;
            if ( ! ce->is_valid() ) continue;
            std::string ce_id = it->first;
            outputfile << ce_id << FIELD_SEPARATOR << ce->get() << FIELD_SEPARATOR << ce->get_creation_date() << FIELD_SEPARATOR << ce->get_ttl() << "\n";
        }
        outputfile.close();
        logger->debug(_("All cache entries were saved at ") + dirname + "/" + filename);
        return 0;
    }
    return 1;
}
