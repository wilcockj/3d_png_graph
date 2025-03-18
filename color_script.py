def parse_color_file(filename):
    colors = []
    with open(filename, 'r') as file:
        for line in file:
            parts = line.strip().split('\t')
            if len(parts) == 3:
                r, g, b = map(int, parts[0].split())
                name = parts[2].replace(" ", "_")  # Replace spaces with underscores for C compatibility
                if len(colors) > 0:
                    _, old_r, old_g, old_b = colors[-1]
                    if colors[-1] and (r,g,b) == (old_r,old_g,old_b):
                        colors.pop()

                colors.append((name, r, g, b))
                # check if previous is the same colors and has underscores

    return colors

def generate_c_array(colors):
    c_code = "typedef struct { const char* name; unsigned char r, g, b; } Color;\n\n"
    c_code += "Color colors[] = {\n"
    for name, r, g, b in colors:
        c_code += f'    {{ "{name}", {r}, {g}, {b} }},\n'
    c_code += "};\n\n"
    c_code += f"const int color_count = {len(colors)};\n\n"
    
    c_code += "const char* find_closest_color(unsigned char r, unsigned char g, unsigned char b) {\n"
    c_code += "    int min_distance = 195076; // Maximum possible distance in RGB space\n"
    c_code += "    const char* closest_color = \"Unknown\";\n"
    c_code += "    for (int i = 0; i < color_count; i++) {\n"
    c_code += "        int dr = colors[i].r - r;\n"
    c_code += "        int dg = colors[i].g - g;\n"
    c_code += "        int db = colors[i].b - b;\n"
    c_code += "        int distance = dr * dr + dg * dg + db * db;\n"
    c_code += "        if (distance < min_distance) {\n"
    c_code += "            min_distance = distance;\n"
    c_code += "            closest_color = colors[i].name;\n"
    c_code += "        }\n"
    c_code += "    }\n"
    c_code += "    return closest_color;\n"
    c_code += "}\n"
    return c_code

if __name__ == "__main__":
    input_file = "rgb.txt"  # Change this to your file name
    colors = []
    with open(input_file, 'r') as file:
        for line in file:
            parts = line.split('\t')
            name = parts[0]
            color = parts[1][1:]

            print(color)
            r = int(color,16) >> 16 & 0xFF
            g = int(color,16) >> 8 & 0xFF
            b = int(color,16) & 0xFF
            print(r,g,b)


            print(parts)
            colors.append((name,r,g,b))

    #colors = parse_color_file(input_file)
    c_code = generate_c_array(colors)
    
    with open("colors.c", "w") as output_file:
        output_file.write(c_code)
    
    print("C array saved to colors.c")

