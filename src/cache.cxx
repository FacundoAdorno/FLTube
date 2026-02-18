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

CacheEntry::CacheEntry(std::string value, unsigned int TTL): value(value) {
    ttl = (TTL > 0) ? TTL : DEFAULT_ENTRY_TTL;
    creation_date = time(0);    // Set current datetime.
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
