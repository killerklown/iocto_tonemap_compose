//
// LICENSE:
//
// Copyright (c) 2016 -- 2017 Fabio Pellacini
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "image.h"

// needed for image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// needed for image writing
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

image4f load_image4f(const std::string& filename) {
	int x = 0;
	int y = 0;
	int n = 0;
	const char *pippo = filename.c_str();
	float *p = stbi_loadf(pippo, &x, &y, &n, 4);

	image4f img = image4f(x, y);

	for (int i = 0; i < x*y; i++)
	{
		int pix = i * 4;
		vec4f vec = vec4f();
		vec.x = (float)p[pix];
		vec.y = (float)p[pix + 1];
		vec.z = (float)p[pix + 2];
		vec.w = (float)p[pix + 3];
		img.pixels[i] = vec;
	}

	return img;
}

image4b load_image4b(const std::string& filename) {
	int x = 0;
	int y = 0;
	int n = 0;
	const char *fn = filename.c_str();
	unsigned char *p = stbi_load(fn, &x, &y, &n, 4);

	image4b img = image4b(x, y);

	for (int i = 0; i < x*y; i++)
	{
		int pix = i * 4;
		vec4b vec = vec4b();
		vec.x = p[pix];
		vec.y = p[pix + 1];
		vec.z = p[pix + 2];
		vec.w = p[pix + 3];
		img.pixels[i] = vec;
	}

	return img;
}

void save_image(const std::string& filename, const image4f& img) {
	const char *filenamec = filename.c_str();
	auto x = img.width;
	auto y = img.height;
	float *data = new float[x * y * 4];
	for (int i = 0; i < x * y; i++)
	{
		int j = i * 4;
		vec4f px = img.pixels[i];
		data[j] = (float)px.x;
		data[j + 1] = (float)px.y;
		data[j + 2] = (float)px.z;
		data[j + 3] = (float)px.w;
	}
	stbi_write_hdr(filenamec, x, y, 4, data);
}

void save_image(const std::string& filename, const image4b& img) {
	const char *filenamec = filename.c_str();
	auto x = img.width;
	auto y = img.height;
	unsigned char *data = new unsigned char[x * y * 4];
	for (int i = 0; i < x * y; i++) {
		int j = i * 4;
		vec4b px = img.pixels[i];
		data[j] = px.x;
		data[j + 1] = px.y;
		data[j + 2] = px.z;
		data[j + 3] = px.w;
	}
    stbi_write_png(filenamec, x, y, 4, data, x*4);
}

float filmic(float x)
{
	float N = (2.51f * pow(x, 2.0f)) + (0.03f * x);
	float D = (2.43f * pow(x, 2.0f) + (0.59f * x) + 0.14f);
	return (N / D);
}

image4b tonemap(
	const image4f& hdr, float exposure, bool use_filmic, bool no_srgb) {
    
    image4b *tonemapped_img = new image4b(hdr.width, hdr.height);

    for(int i =  0; i < hdr.pixels.size(); i++) // for all pixels in input image...
    {
        float gamma = 2.2f; // srgb gamma
        if(no_srgb)
        {
            gamma = 1.0f; // no-srgb gamma
        }

        vec4f pix = hdr.pixels[i];
        auto pixelsb = vec4b();
        
        pix.x = pix.x * pow(2.0f, exposure); // apply exposure to R
        pix.y = pix.y * pow(2.0f, exposure); // apply exposure to G
        pix.z = pix.z * pow(2.0f, exposure); // apply exposure to B
        
        if(use_filmic)
        {
			pix.x = filmic(pix.x); // apply filmic to R
			pix.y = filmic(pix.y); // apply filmic to G
			pix.z = filmic(pix.z); // apply filmic to B
        }
        
        pixelsb.x = (unsigned char)max(0.0f, fmin(255, pow(pix.x, (1.0f/gamma))*255.0f)); // gamma correction on R + cast
        pixelsb.y = (unsigned char)max(0.0f, fmin(255, pow(pix.y, (1.0f/gamma))*255.0f)); // gamma correction on G + cast
        pixelsb.z = (unsigned char)max(0.0f, fmin(255, pow(pix.z, (1.0f/gamma))*255.0f)); // gamma correction on B + cast
        pixelsb.w = (unsigned char)max(0.0f, pix.w*255.0f); // alpha value cast

        tonemapped_img->pixels[i] = pixelsb; // insert tonemapped pixel into output image
    }
    
    return *tonemapped_img;
}

image4b compose(
	const std::vector<image4b>& imgs, bool premultiplied, bool no_srgb) {

	image4b compositedImg = imgs[0];

	for (int i = 1; i < imgs.size(); i++) // for all images in input vector...
	{
		image4b Aimg = imgs[i];
		image4b Bimg = compositedImg;

		for (int j = 0; j < Aimg.pixels.size(); j++) // for all pixels in the i-th image...
		{
			vec4b pixA = Aimg.pixels[j];
			unsigned char rgbA[3] = { pixA.x, pixA.y, pixA.z }; // { r, g, b }
			unsigned char alphaA = pixA.w; // use this alpha value for no_srgb (unsigned char)
			float alfaA = (float)alphaA / 255; // use alpha value this for srgb (float)

			vec4b pixB = Bimg.pixels[j];
			unsigned char rgbB[3] = { pixB.x, pixB.y, pixB.z }; // { r, g, b }
			unsigned char alphaB = pixB.w; // use this alpha value for no_srgb (unsigned char)
			float alfaB = (float)alphaB / 255; // use alpha value this for srgb (float)

			unsigned char rgbE[3] = { 0, 0, 0 };
			auto pixE = vec4b();

			if (no_srgb)
			{
				if (premultiplied) // non-srgb premultiplied
				{
					unsigned char pmAr = alphaA * rgbA[0];
					unsigned char pmAg = alphaA * rgbA[1];
					unsigned char pmAb = alphaA * rgbA[2];

					unsigned char pmBr = alphaB * rgbB[0];
					unsigned char pmBg = alphaB * rgbB[1];
					unsigned char pmBb = alphaB * rgbB[2];

					pixE.x = pmAr + ((1 - alphaA) * pmBr);
					pixE.y = pmAg + ((1 - alphaA) * pmBg);
					pixE.z = pmAb + ((1 - alphaA) * pmBb);
				}
				else // non-srgb non-premultiplied
				{
					pixE.x = (alphaA * rgbA[0]) + ((1 - alphaA) * rgbB[0]);
					pixE.y = (alphaA * rgbA[1]) + ((1 - alphaA) * rgbB[1]);
					pixE.z = (alphaA * rgbA[2]) + ((1 - alphaA) * rgbB[2]);
				}
				pixE.w = (alphaA + (1 - alphaA) * alphaB); // non-srgb alpha value
			}
			else
			{
				if (premultiplied) // srgb premultiplied
				{
					float pmAr = alfaA * rgbA[0];
					float pmAg = alfaA * rgbA[1];
					float pmAb = alfaA * rgbA[2];

					float pmBr = alfaB * rgbB[0];
					float pmBg = alfaB * rgbB[1];
					float pmBb = alfaB * rgbB[2];

					pixE.x = pmAr + ((1 - alfaA) * pmBr);
					pixE.y = pmAg + ((1 - alfaA) * pmBg);
					pixE.z = pmAb + ((1 - alfaA) * pmBb);
				}
				else // srgb non-premultiplied
				{
					pixE.x = (alfaA * rgbA[0]) + ((1 - alfaA) * rgbB[0]);
					pixE.y = (alfaA * rgbA[1]) + ((1 - alfaA) * rgbB[1]);
					pixE.z = (alfaA * rgbA[2]) + ((1 - alfaA) * rgbB[2]);
				}
				pixE.w = (alfaA + (1 - alfaA) * alfaB) * 255; // srgb alpha value
			}

			compositedImg.pixels[j] = pixE; // insert composited pixel into output image
		}
	}
	return compositedImg;
}
