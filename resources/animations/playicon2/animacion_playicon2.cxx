#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Timer.H>
#include <FL/Fl_Input.H>
#include <cstdio>
#include <vector>
#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Device.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/fl_draw.H>
#include <cassert>
#include <vector>
#include <iostream>
#include <string>

class Animation {
public:
    Animation(Fl_Box* box, const std::vector<const char*>& image_files)
    : box(box), images(), current_frame(0) {
        for (const char* file : image_files) {
            auto image = load_image(file);
            check_image_details(image);
            auto transparent_image = get_transparent_image(image);
            printf("Procese imagen %s\n", file);
            images.push_back(transparent_image);
        }
        box->image(images[current_frame]);
    }

    void next_frame() {
        box->hide();
        box->show();
        current_frame = (current_frame + 1) % images.size();
        printf("Frame Nº %zu\n", current_frame);
        box->image(images[current_frame]);
        box->redraw();
    }

private:
    Fl_Box* box;
    std::vector<Fl_RGB_Image*> images;
    size_t current_frame;

    Fl_RGB_Image* load_image(const char* path) {
        // Cargar la imagen PNG
        Fl_PNG_Image* png_image = new Fl_PNG_Image(path);

        // Verificar si la imagen se cargó correctamente
        if (png_image->w() == 0 || png_image->h() == 0) {
            std::cerr << "Error al cargar la imagen: " << path << std::endl;
            delete png_image; // Liberar la memoria si no se cargó
            return nullptr;
        }

        return static_cast<Fl_RGB_Image*>(png_image);
    }


    Fl_RGB_Image* get_transparent_image(const Fl_RGB_Image* image) {
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

    void check_image_details(Fl_Image* image) {
        if (!image) {
            std::cerr << "Imagen nula" << std::endl;
            return;
        }

        std::cout << "Detalles de la imagen:" << std::endl;
        std::cout << "Ancho: " << image->w() << std::endl;
        std::cout << "Alto: " << image->h() << std::endl;
        std::cout << "Profundidad: " << image->d() << std::endl;

        // Verificar si la imagen tiene canal alfa
        if (image->d() == 4) {
            std::cout << "La imagen tiene canal alfa" << std::endl;
        } else {
            std::cout << "La imagen NO tiene canal alfa" << std::endl;
        }
    }
};

void timer_callback(void* data) {
    Animation* animation = static_cast<Animation*>(data);
    animation->next_frame();
    Fl::repeat_timeout(0.1, timer_callback, data); // Cambia el tiempo según sea necesario
}


Animation* animation;

void stop_start_animation_cb(Fl_Widget* w) {
    // TODO
}


int main() {
    Fl_Window* window = new Fl_Window(800, 600, "Animación FLTK");
    Fl_Box* background = new Fl_Box(0,0,800,600);
    background->image(new Fl_PNG_Image("/home/facujosegrabacion/Imágenes/gaucho_compu.png"));
    Fl_Box* box_bg = new Fl_Box(99, 99, 400, 300);
    box_bg->label("Caja Superior");
    box_bg->labelsize(20);
    box_bg->color(FL_WHITE);
    Fl_Box* box = new Fl_Box(100, 100, 400, 300);

    std::vector<const char*> image_files = {};
    std::string animation_prefix = "playicon2_";

    for (int i = 0; i<=30; i++) {
        std::string filepath;
        if ( i < 10 )   filepath = animation_prefix + "000" + std::to_string(i) + ".png";
        else filepath = animation_prefix + "00" + std::to_string(i) + ".png";

        printf("Archivo cargado: %s\n", filepath.c_str());
        image_files.push_back(strdup(filepath.c_str()));
    }

    for ( int j = 0; j < image_files.size(); j++) {
        printf("Archivo guardado en array: %s\n", image_files[j]);
    }

    animation = new Animation(box, image_files);
    Fl::add_timeout(0.1, timer_callback, &animation); // Inicia el temporizador

    Fl_Button* button = new Fl_Button(400, 300, 50, 50, "Hola");
    Fl_Input* input = new Fl_Input(300, 360, 200, 20, "Ingrese su texto: ");

    window->end();
    window->show();
    return Fl::run();

    //Command to compile: fltk-config --use-images --compile animacion_playicon2.cxx
}
