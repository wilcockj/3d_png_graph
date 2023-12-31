#include "raylib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t *drawn_pixel_map;
Color *color_list;
Vector3 *cubePosition;

bool color_in_list(Color cur_color) {
  // use cur color to see what to check
  unsigned int index = (cur_color.r / 255.0f) + (cur_color.g / 255.0f) * (256) +
                       (cur_color.b / 255.0f) * (256 * 256);
  unsigned int byte_index = index / (8);
  unsigned int cur_bitfield = drawn_pixel_map[byte_index];
  return cur_bitfield >> (index - (byte_index * 8));
}

int main(void) {
  printf("intializing\n");

  printf("Loading image\n");
  Image image = LoadImage("oceansmall.png"); // Replace with your image path
  // Initialization
  drawn_pixel_map = malloc((256 * 256 * 256) / (8 * sizeof(uint8_t)));
  color_list = malloc(image.height * image.width * sizeof(Color));
  cubePosition = malloc(image.height * image.width * sizeof(Vector3));

  const int screenWidth = 800;
  const int screenHeight = 450;
  const float scale = 0.01f; // Scale factor for the 3D space
  const float graph_limit = 2;
  printf("intialized\n");

  InitWindow(screenWidth, screenHeight, "3D Graph from Image");

  // Load image

  // Set up the camera
  Camera camera = {0};
  camera.position = (Vector3){2.0f, 2.0f, 2.0f};
  camera.target = (Vector3){0.5f, 0.5f, 0.5f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  printf("making color list\n");

  int color_cnt = 0;
  printf("populating color list\n");
  for (int y = 0; y < image.height; y++) {
    for (int x = 0; x < image.width; x++) {
      Color color = GetImageColor(image, x, y);
      if (!color_in_list(color)) {
        color_list[color_cnt] = color;
        Vector3 pos;
        pos.x = (float)color.r / 255.0f * graph_limit;
        pos.y = (float)color.g / 255.0f * graph_limit;
        pos.z = (float)color.b / 255.0f * graph_limit;
        cubePosition[color_cnt++] = pos;
      }
    }
  }
  printf("Found %d unique colors\n", color_cnt);

  while (!WindowShouldClose()) {
    // Update
    UpdateCamera(&camera, CAMERA_ORBITAL); // Update camera

    // Start Drawing
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);

    for (int i = 0; i < color_cnt; i++) {

      Color color = color_list[i];
      Vector3 pos = cubePosition[i];
      DrawCube(pos, scale, scale, scale, color);
    }

    EndMode3D();
    DrawText("3D Graph from Image RGB", 10, 10, 20, BLACK);
    DrawFPS(10, 30);
    EndDrawing();
  }

  // De-Initialization
  UnloadImage(image); // Unload image
  CloseWindow();      // Close window

  return 0;
}
