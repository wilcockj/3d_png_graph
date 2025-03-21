#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>

struct image_info {
  size_t color_cnt;
  size_t num_pixels;
  Image *target_image;
  Texture2D *target_texture;
  Matrix **transform_list;
  Color *color_list;
  uint8_t *drawn_pixel_map;
  Color *palette;
  const char **palette_color_names;
  size_t palette_len;
};

void Draw_Image_In_Region(Texture2D tex, Rectangle region);

bool color_in_list(Color cur_color, uint8_t *drawn_pixel_map);

int populate_color_list(Image target_image, Color *color_list,
                        uint8_t *drawn_pixel_map);

void load_transforms_from_color_list(Matrix *transform_list, Color *color_list,
                                     int color_cnt, unsigned int quadrant);

void process_image(struct image_info *info, Image target_image);
