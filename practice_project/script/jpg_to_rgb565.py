import os
import sys
from PIL import Image

def convert_to_rgb565_byte_pairs(image_path, output_c_path=None, resize_dims=None, big_endian=True):
    """
    Converts a JPG image to an RGB565 byte array format (0xXX, 0xXX for each pixel).
    
    :param image_path: Path to the input JPG image
    :param output_c_path: Path to write the .c file (defaults to image name + .c)
    :param resize_dims: Optional tuple (width, height) to resize the image
    :param big_endian: If True, outputs MSB first (0xHH, 0xLL). If False, LSB first (0xLL, 0xHH).
    """
    if not os.path.exists(image_path):
        print(f"Error: Image '{image_path}' not found.")
        return

    # Open image and ensure it's in RGB mode
    img = Image.open(image_path).convert('RGB')
    
    # Optional resize (useful for small TFT screens)
    if resize_dims:
        img = img.resize(resize_dims)
        
    width, height = img.size
    pixels = img.load()
    
    # Base output name on the input filename
    base_name = os.path.splitext(os.path.basename(image_path))[0]
    array_name = f"image_{base_name.replace('-', '_').replace(' ', '_')}"
    
    if not output_c_path:
        output_c_path = f"{base_name}_rgb565_bytes.c"
        
    c_array_data = []
    
    # Loop through pixels and pack them
    for y in range(height):
        row_hex = []
        for x in range(width):
            r, g, b = pixels[x, y]
            
            # Scale 8-bit to 5-bit (Red), 6-bit (Green), and 5-bit (Blue)
            r_5 = (r >> 3) & 0x1F
            g_6 = (g >> 2) & 0x3F
            b_5 = (b >> 3) & 0x1F
            
            # Pack into 16-bit value: rrrrrggggggbbbbb
            rgb565 = (r_5 << 11) | (g_6 << 5) | b_5
            
            # Split into High and Low bytes
            high_byte = (rgb565 >> 8) & 0xFF
            low_byte = rgb565 & 0xFF
            
            # Order bytes based on desired endianness (most TFT screens prefer Big Endian / MSB first)
            if big_endian:
                row_hex.append(f"0x{high_byte:02X}, 0x{low_byte:02X}")
            else:
                row_hex.append(f"0x{low_byte:02X}, 0x{high_byte:02X}")
            
        c_array_data.append("    " + ", ".join(row_hex))

    # Write out the C file
    with open(output_c_path, 'w') as f:
        f.write("#include <stdint.h>\n\n")
        f.write(f"// Image Dimensions: {width}x{height}\n")
        f.write(f"const uint16_t {array_name}_width = {width};\n")
        f.write(f"const uint16_t {array_name}_height = {height};\n\n")
        f.write(f"// RGB565 representation stored as individual bytes\n")
        f.write(f"const uint8_t {array_name}[{width * height * 2}] = {{\n")
        f.write(",\n".join(c_array_data))
        f.write("\n};\n")
        
    print(f"Success! Saved {width}x{height} image to '{output_c_path}' as array '{array_name}'.")

if __name__ == "__main__":
    # Example Usage: Replace with your actual image path and target dimension.
    # big_endian=True writes High-Byte, Low-Byte (common for SPI displays)
    convert_to_rgb565_byte_pairs("shot1.png",output_c_path="shot1.h", resize_dims=(40, 40), big_endian=True)