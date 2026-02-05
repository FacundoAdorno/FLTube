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


#include "../include/fltk_animation.h"
#include <filesystem>

SimpleAnimation::SimpleAnimation(Fl_Box* box, std::string path_compressed_animation, std::string extraction_path, int count_of_frames, FPS_MS animation_fps): PATH_TO_COMPRESSED_ANIMATION(path_compressed_animation), PATH_TO_EXTRACTED_FRAMES(extraction_path), NUMBER_OF_FRAMES(count_of_frames), FPS(animation_fps), target_box_for_animation(box), current_frame(0), images({}), has_errors(false)
    {
        if (! std::filesystem::exists(path_compressed_animation)) {
            fprintf(stderr, "[WARN] The animation file %s does not exists... Aborting processing.\n", path_compressed_animation.c_str());
            has_errors = true;
            return;
        }

        std::vector<std::string> image_files = {};
        if (extract_animation_files(&image_files) > 0) {
            fprintf(stderr, "[ERROR] Aborting extraction of animation at path %s due to an error.\n", path_compressed_animation.c_str());
            has_errors = true;
        } else {
            for (const std::string file : image_files) {
                auto image = load_image(file.c_str());
                if (image == nullptr) {
                    has_errors = true;
                    return;
                }
                auto transparent_image = get_transparent_image(image);
                // printf("Image processed: %s\n", file.c_str());
                images.push_back(transparent_image);
            }
            if (box != nullptr) this->start();
        }
    };


void SimpleAnimation::animate(void* data){
    SimpleAnimation* animation = static_cast<SimpleAnimation*>(data);
    if (animation->get_status() == PLAY_STATUS::STOPPED) {
        // Stop sending the repeat timeout if animation stopped...
        return;
    }

    if (animation->get_status() == PLAY_STATUS::PLAY) {
        animation->next_frame();
    }
    // Add a new timeout (a repeat_timeout) for this animations...
    Fl::repeat_timeout(animation->getFrameDuration(), animation->animate, animation);
}

void SimpleAnimation::update_frame_position(int change_frame_factor) {
    //Doesn't update the frame if animation is stopped or paused....
    if (this->play_status == PLAY_STATUS::STOPPED || this->play_status == PLAY_STATUS::PAUSED)  return;
    this->target_box_for_animation->hide();
    this->target_box_for_animation->show();
    current_frame = (current_frame + change_frame_factor) % images.size();
    // printf("Frame Nº %zu\n", current_frame);
    this->target_box_for_animation->image(images[current_frame]);
    this->target_box_for_animation->redraw();
}

void SimpleAnimation::next_frame() {
    if (! has_box()) {
        return;
    }
    this->update_frame_position(1);
}

void SimpleAnimation::previous_frame() {
    if (! has_box()) {
        return;
    }
    this->update_frame_position(-1);
}

void SimpleAnimation::play() {
    this->play_status = PLAY_STATUS::PLAY;
    this->target_box_for_animation->show();
}

void SimpleAnimation::stop() {
    this->play_status = PLAY_STATUS::STOPPED;
    reset();
}

void SimpleAnimation::pause_unpause() {
    if (this->play_status == PLAY_STATUS::STOPPED) return;

    this->play_status = (this->play_status == PLAY_STATUS::PAUSED) ? PLAY_STATUS::PLAY : PLAY_STATUS::PAUSED;
}

PLAY_STATUS SimpleAnimation::get_status() {
    return this->play_status;
}

bool SimpleAnimation::is_active() {
    return get_status() == PLAY_STATUS::PLAY || get_status() == PLAY_STATUS::PAUSED;
}

void SimpleAnimation::start() {
    reset();
    play();
    this->target_box_for_animation->image(images[current_frame]);
    Fl::add_timeout(this->getFrameDuration(), this->animate, this);
}

void SimpleAnimation::reset() {
    this->current_frame = 0;
}

void SimpleAnimation::clear_box() {
    if (this->has_box()) {
        this->target_box_for_animation->image(nullptr);
        this->target_box_for_animation->hide();
        this->target_box_for_animation->redraw();
        Fl::flush();
    }
    this->target_box_for_animation = nullptr;
}

void SimpleAnimation::set_box(Fl_Box* box) {
    if (target_box_for_animation != nullptr) {
        clear_box();
    }
    target_box_for_animation = box;
    reset();
}

Fl_RGB_Image* SimpleAnimation::load_image(const char* path) {
    // Load PNG Image
    Fl_PNG_Image* png_image = new Fl_PNG_Image(path);

    // Check if image was loaded correctly.
    if (png_image->w() == 0 || png_image->h() == 0) {
        std::cerr << "[E] Cannot load the image: " << path << std::endl;
        delete png_image; // Free memory if not loaded...
        return nullptr;
    }

    return static_cast<Fl_RGB_Image*>(png_image);
}


Fl_RGB_Image* SimpleAnimation::get_transparent_image(const Fl_RGB_Image* image) {
    if (!image) return nullptr;

    // Check if image already has an alpha channel
    if (image->d() == 4) {
        return new Fl_RGB_Image(reinterpret_cast<const unsigned char*>(image->data()[0]), image->w(), image->h(), 4);
    }

    // Create a new image with alpha channel
    uchar* data = new uchar[image->w() * image->h() * 4];
    for (int y = 0; y < image->h(); y++) {
        for (int x = 0; x < image->w(); x++) {
            int index = (y * image->w() + x) * 4;
            int original_index = (y * image->w() + x) * 3;

            // Copy RGB values
            data[index] = image->data()[0][original_index];
            data[index + 1] = image->data()[0][original_index + 1];
            data[index + 2] = image->data()[0][original_index + 2];

            // Set full opacity by default
            data[index + 3] = 255;
        }
    }

    return new Fl_RGB_Image(data, image->w(), image->h(), 4);
}

int SimpleAnimation::extract_animation_files(std::vector<std::string>* images_filepaths) {
    /* Select which attributes we want to restore. */
    int flags;
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    const char* TARGET_DIRECTORY = PATH_TO_EXTRACTED_FRAMES.c_str();
    const char* PATH_TO_TARGZ = PATH_TO_COMPRESSED_ANIMATION.c_str();

    struct archive* source_archive = archive_read_new();
    archive_read_support_format_tar(source_archive);
    archive_read_support_filter_gzip(source_archive);

    struct archive* target_archive = archive_write_disk_new();
    archive_write_disk_set_options(target_archive, flags);
    archive_write_disk_set_standard_lookup(target_archive);

    // Create extraction directories for compressed files.
    try{
        std::filesystem::create_directories(TARGET_DIRECTORY);
    } catch (const std::filesystem::filesystem_error &ex) {
        printf("[E] Cannot create directory %s. Aborting file extraction %s.\n", TARGET_DIRECTORY, PATH_TO_TARGZ);
        printf("[DEBUG] %s",ex.what());
        return 3;
    }


    if (archive_read_open_filename(source_archive, PATH_TO_TARGZ, 10240) != ARCHIVE_OK) {
        fprintf(stderr, "[E] Cannot read the file %s.\n", PATH_TO_TARGZ);
    }

    // printf("[I] Decompressing content inside file %s.\n", PATH_TO_TARGZ);
    struct archive_entry* entry;
    int r, count_processed;
    r = ARCHIVE_OK;
    count_processed = 0;

    while ( r == ARCHIVE_OK) {
        r = archive_read_next_header(source_archive, &entry);
        if (r == ARCHIVE_EOF) {
            break;
        } else if (r < ARCHIVE_WARN) {      //FAILED OR FATAL CODE
            fprintf(stderr, "[E] Cannot read next entry of compressed file:\n%s\n", archive_error_string(source_archive));
            exit(1);
        } else if (r < ARCHIVE_OK) {        //RETRY OR WARN CODE
            fprintf(stderr, "[W] %s\n", archive_error_string(source_archive));
        }


        //Copy source data to target output file
        const char* entry_name = archive_entry_pathname(entry);
        //printf("[I] Proccessing entry %s...\n", entry_name);
        char entry_pathname[512];
        snprintf(entry_pathname, sizeof(entry_pathname), "%s/%s", TARGET_DIRECTORY, entry_name);
        archive_entry_set_pathname(entry, entry_pathname);
        r = archive_write_header(target_archive, entry);
        if (r < ARCHIVE_WARN) {      //FAILED OR FATAL CODE
            fprintf(stderr, "[E] Cannot write header in target file:\n%s\n", archive_error_string(source_archive));
            exit(1);
        } else if (r < ARCHIVE_OK) {        //RETRY OR WARN CODE
            fprintf(stderr, "[W] %s\n", archive_error_string(source_archive));
        }
        if (archive_entry_size(entry) > 0) {
            // COPY DATA SECTION
            int read_res = ARCHIVE_OK, write_res = ARCHIVE_OK;
            const void* buff;
            size_t size;
            la_int64_t offset;

            while (read_res != ARCHIVE_EOF) {
                //Leyendo bytes
                read_res = archive_read_data_block(source_archive, &buff, &size, &offset);

                if (read_res < ARCHIVE_WARN) {      //FAILED OR FATAL CODE
                    fprintf(stderr, "[E] Cannot read file's entry:\n%s\n", archive_error_string(source_archive));
                    return 1;
                } else if (read_res < ARCHIVE_OK) {        //RETRY OR WARN CODE
                    fprintf(stderr, "[W] Aborting entry processing: %s\n", archive_error_string(source_archive));
                    break;
                }
                // Copying bytes...
                write_res = archive_write_data_block(target_archive, buff, size, offset);
                images_filepaths->push_back(std::string(entry_pathname));
                if (write_res < ARCHIVE_OK) {
                    fprintf(stderr, "[W] Cannot copy entry data: %s\n", archive_error_string(target_archive));
                    break;
                }
            }

        }

        r = archive_write_finish_entry(target_archive);
        if (r < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(target_archive));
        if (r < ARCHIVE_WARN)
            return 1;

        count_processed++;

    }

    //printf("[I] Finish processing %d entry(s) inside compressed file.\n", count_processed);

    archive_read_close(source_archive);
    archive_read_free(source_archive);
    archive_write_close(target_archive);
    archive_write_free(target_archive);
    return 0;
}
