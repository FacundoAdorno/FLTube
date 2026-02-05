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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl.H>
#include <string>
#include <vector>
#include <iostream>
#include <archive.h>
#include <archive_entry.h>
#include <cstdio>
#include <filesystem>

// It indicates how long a millisecond lasts according to the number of FPS.
enum FPS_MS {
    _10FPS = 100,
    _20FPS = 50,
    _30FPS = 33,
    _60FPS = 17     //16,6
};

enum PLAY_STATUS {
    PLAY, PAUSED, STOPPED
};

/* Add a Simple PNG Animation to an existing Fl_Box. Receives a compressed tar.gz file containing all the animations in an ordered way.
 * An animation needs for a box (a pointer to an existing Fl_Box) to be displayed, and can be replaced later for any other box.
 */
class SimpleAnimation {
    private:
        /*  Path to a tar.gz file that all PNG animations. Every file must be prefixed with a string (for example "animation_"), followed by four digits (example: animation_0000.png, animation_0001.png, etc.), and ended with .png, creating a serie of ordered files. */
        const std::string PATH_TO_COMPRESSED_ANIMATION;

        /* Path to extracted files of the animation. */
        const std::string PATH_TO_EXTRACTED_FRAMES;

        /* Quantity of frames that the animation has. */
        const int NUMBER_OF_FRAMES;

        Fl_Box* target_box_for_animation;

        /* Used for calculate the next frame in the sequence.*/
        int current_frame;

        /* List of PNG images that compose the animation. */
        std::vector<Fl_RGB_Image*> images;

        /* This variable keeps the play status of the animation:
         *  (1) If STOPPED, the animation must be started from the first frame.
         *  (2) If PAUSED, the animation can be unpaused from the frame where was paused.
         *  (3) If PLAYING, the animation can be PAUSED or STOPPED. */
        PLAY_STATUS play_status;

        /* The frames per second (FPS) that this animation was designed for. */
        FPS_MS FPS;

        Fl_RGB_Image* load_image(const char* path);

        Fl_RGB_Image* get_transparent_image(const Fl_RGB_Image* image);

        /* Extract the animation images inside the .tar.gz file passed as parameter at path @PATH_TO_EXTRACTED_FRAMES. */
        int extract_animation_files(std::vector<std::string>* images_filepaths);

        /*  Add or decrement the frame position in the indicated factor. In example, add a frame when @change_frame_factor = 1, or decrement a frame when @change_frame_factor = -1.*/
        void update_frame_position(int change_frame_factor);

        /* This function is a Fl_Timeout_Handler that manages the animation. You can use with */
        static void animate(void* data);

        /* Flag to know if animation cannot be used for some error when decompress or loading images... */
        bool has_errors;

    public:
        SimpleAnimation(Fl_Box* box, std::string path_compressed_animation, std::string extraction_path, int count_of_frames, FPS_MS animation_fps);
        ~SimpleAnimation();

        /* Move the position of the animation 1 frame ahead. */
        void next_frame();
        /* Move the position of the animation 1 frame behind. */
        void previous_frame();

        /* Start animation from frame 0 and set image's box for that frame (if box exists).
         * Also, configuring the Fl_Timeout_Handler for a continuos animation. */
        void start();
        /* Set frame position to the begining (frame 0). */
        void reset();
        /* Update the play status of the animation to @PLAY_STATUS::PLAY. */
        void play();
        /* Update the play status of the animation to @PLAY_STATUS::STOPPED, and reset the animation. */
        void stop();
        /*  If the animation is not stopped, pause (@PLAY_STATUS::PAUSED) or unpause (@PLAY_STATUS::PLAY) animations. */
        void pause_unpause();
        /* Return the current play status of the animation. */
        PLAY_STATUS get_status();
        /*  Return if the animation is currently being played or paused. */
        bool is_active();

        /* Clear the box used by this animation. */
        void clear_box();

        /* Set a new box and reset the animation from start. */
        void set_box(Fl_Box* box);

        /*  Returns if a box is set for this animation. */
        bool has_box() {
            return this->target_box_for_animation != nullptr;
        }

        /*  Return the current box used currently to display the animation. */
        Fl_Box* get_box() {
            return target_box_for_animation;
        }

        /* Returns the duration in seconds for a frame for this animation. */
        float getFrameDuration() {
            return SimpleAnimation::FPStoFloat(this->FPS);
        }

        /* Returns the duration in seconds for an specific FPS (@FPS_MS) expression. */
        static float FPStoFloat(FPS_MS fps) {
            return fps / 1000.0f;
        }

        /*  Specify if the animation could be loaded correctly or errors occurred when doing this process. */
        bool fail_to_load() {
            return this->has_errors;
        }
};

#endif
