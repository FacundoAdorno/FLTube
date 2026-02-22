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

#ifndef FLCACHE_H
#define FLCACHE_H

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <ctime>
#include <fstream>
#include <vector>
#include "fltube_utils.h"

/* This entity represents a general cache entry. It has a string value, a creation date, a time to live (expressed in seconds),
 *  and a "valid" flag. Once created, the cache entry values cannot be changed.
 *  A cache entry becames invalid automatically when reachs its invalidation date, it means: current_date >= (creation_date + ttl).
 */
class CacheEntry {
private:
    std::string     value;
    time_t          creation_date;  //This is a UTC epoch date
    unsigned int    ttl;            //Time to live of this entry.
    bool            still_valid;

public:
    //Default time to live of an entry is 6 hours, expressed in seconds.
    const static int   DEFAULT_ENTRY_TTL = 6 * 60 * 60;

    /*  Create an entry with a value, an a custom time to live in seconds. An empty value make this an invalid entry. */
    CacheEntry(std::string value, unsigned int TTL = DEFAULT_ENTRY_TTL, unsigned long int custom_creation_date = 0);

    static std::string   EMPTY_VALUE;

    /*  Arbitrarily, mark this entry as invalid, even if is valid. */
    void mark_invalid() {
        still_valid = false;
    }

    /* Get if current entry is valid (by default, is valid if current time is between the creation_date and creation_date + ttl). */
    bool is_valid();

    /*  If entry is valid, return the value of this entry. Otherwise, return a empty string. */
    std::string get();

    /* Returns the expiration date for this cache entry.  */
    time_t get_expiration();

    time_t get_creation_date () {
        return creation_date;
    }

    /* TTL: Time to live of this entry. */
    unsigned int get_ttl() {
        return ttl;
    }

    //TODO  Add a way to update a cache entry value?
};

/*  This general cache is a memory cache. Add, update, select or remove cache entries. */
class GeneralCache {
protected:
    std::unique_ptr<std::map<std::string, CacheEntry*>> entries;
    std::shared_ptr<TerminalLogger> logger;

    unsigned int cache_entry_ttl;

    /* Returns the @CacheEntry for a specified id. If not exists, a nullptr is returned. */
    CacheEntry* search(std::string id);
    /* Hook method used for Class Constructor to load data from the corresponding datasource. */
    virtual int load();

    /* This function must be implemented for every subclass to "save" the data in any form required. */
    virtual int save();
public:
    GeneralCache(std::shared_ptr<TerminalLogger> const& lgg, unsigned int ttl = CacheEntry::DEFAULT_ENTRY_TTL):
    logger(lgg), entries(std::make_unique<std::map<std::string, CacheEntry*>>()),
    cache_entry_ttl(ttl) {};

    /* Destructor. */
    ~GeneralCache();

    void init();

    void finish();

    /*  ADD a new cache entry, or UPDATES an existing one if its value is different to previous saved. If the entry exists
     *  in the chache, and it is marked as invalid, replace this cache entry with a new valid entry. */
    void add_entry(std::string id, const std::string value);

    /*  Returns an existing cache entry's value, or an empty string (@CacheEntry::EMPTY_VALUE) if it doesn't exist (or it is marked as or became invalid). */
    std::string get_entry_value(std::string id);

    /* Returns true if the entry exists and was deleted succesfully. Besides, the pointer to CacheEntry* is deleted.  */
    bool remove_entry(std::string id);

    /* Returns true if an "id" exists within cache entries and is valid. Otherwise, return false.  */
    bool is_cached(std::string id);

    /* Returns a formatted string representing the hour of expiration of an specified entry. */
    std::string get_cache_expiration_date(std::string id);

    /* Iterate over every cache entry and mark as invalid those who reached its invalidation date. */
    void cleanup();
};

/*  This is like a GeneralCache but save it at your disk for a permanent storage of its data. A cache entry mark as invalid, is not
 *  saved at disk.
 */
class PermanentDiskCache: public GeneralCache {
private:
    const std::string DEFAULT_SAVE_PATH = std::filesystem::temp_directory_path();
    const std::string DEFAULT_SAVE_FILENAME = "cache.txt";
    /*  This is the filesystem directory path where the cache will be stored. */
    std::string dirname;
    /*  This is the filesystem filename for the cache file. */
    std::string filename;
    /* Returns true if directory set to this cache exists and is a directory. Must be used every time cache is saved to disk. */
    bool validate_save_directory();
    /* Returns true if cache filename exists or could be created. Must be used every time cache is saved to disk. */
    bool validate_save_filename();
public:
    static const char FIELD_SEPARATOR = '>';
    /* Total number of fields persisted in each line of the cache file.  */
    static const short int TOTAL_FIELDS_SAVED = 4;

    enum FIELDS_POS { ID, FINAL_URL, CREATION_DATE, TTL };

    PermanentDiskCache(std::shared_ptr<TerminalLogger> const& lgg, unsigned int ttl = CacheEntry::DEFAULT_ENTRY_TTL):
            GeneralCache(lgg,ttl) {};

    /*  Set the path to an existing directory to save the cache file. */
    void set_save_directory_path(std::string dirname_path, std::string filename);

protected:
    /* Load every entry saved at cache file, and only populate the cache with those entries that are valid. */
    int load() override;

    /* Save every valid cache entry to the cache file. The format of every saved line is the following:
     *  ID>VALUE>CREATION_DATE>TTL
     */
    int save() override;
};


#endif      // FLCACHE_H
