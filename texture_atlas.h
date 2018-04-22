/*
 * Copyright 2018 Michael Barkholt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TEXTURE_ATLAS_H_
#define TEXTURE_ATLAS_H_

#include <stdbool.h>

enum TextureAtlas_format {
	TextureAtlas_ALPHA,
	TextureAtlas_INTENSITY,
	TextureAtlas_LUMINANCE_ALPHA,
	TextureAtlas_RGB565,
	TextureAtlas_RGBA4444,
	TextureAtlas_RGB888,
	TextureAtlas_RGBA8888,
	TextureAtlas_UNDEFINED_FORMAT
};
enum TextureAtlas_repeat {
	TextureAtlas_X, TextureAtlas_Y, TextureAtlas_XY, TextureAtlas_NONE, TextureAtlas_UNDEFINED_REPEAT
};
enum TextureAtlas_filter {
	TextureAtlas_NEAREST,
	TextureAtlas_LINEAR,
	TextureAtlas_MIP_MAP,
	TextureAtlas_MIP_MAP_NEAREST_NEAREST,
	TextureAtlas_MIP_MAP_LINEAR_NEAREST,
	TextureAtlas_MIP_MAP_NEAREST_LINEAR,
	TextureAtlas_MIP_MAP_LINEAR_LINEAR,
	TextureAtlas_UNDEFINED_FILTER
};

struct TextureAtlas_atlas {
	/* Pointer to memory containing the first page.*/
	struct TextureAtlas_page* firstPage;
};

struct TextureAtlas_page {
	/* Filename of the image backing this page. */
	char* name;

	/* The width and height of the page image. */
	int width, height;

	/* The format to use for storing the image in memory. Possible values:
	 * Alpha, Intensity, LuminanceAlpha, RGB565, RGBA4444, RGB888, RGBA8888. */
	enum TextureAtlas_format format;

	/* The texture filter minification and magnification settings.
	 * Possible values: Nearest, Linear, MipMap, MipMapNearestNearest,
	 * MipMapLinearNearest, MipMapNearestLinear, MipMapLinearLinear. */
	enum TextureAtlas_filter minificationFilter, magnificationFilter;

	/* The texture wrap settings.
	 * Possible values: x, y, xy, none. */
	enum TextureAtlas_repeat repeat;

	/* Link to the next page, or NULL if this is the last page. */
	struct TextureAtlas_page* next;

	/* The head of the linked list containing all regions of the page. */
	struct TextureAtlas_region* firstRegion;
};

struct TextureAtlas_region {
	/* The page this region belongs to.*/
	struct TextureAtlas_page* page;

	/* The name of the original image file, up to the first underscore.
	 * Underscores denote special instructions to the texture packer. */
	char* name;

	/* True if the region is rotated 90 degrees counter clockwise. */
	bool rotate;

	/* The location of the region within the page. */
	unsigned int x, y;

	/* The size of the region in the page. */
	unsigned int width, height;

	/* Original size of the region, before it was packed. Might be larger than
	 * width/height if whitespace was stripped. */
	unsigned int orignalWidth, orignalHeight;

	/* The amount of whitespace pixels that were stripped from the left and
	 * bottom edges of the image before it was packed. */
	unsigned int offsetX, offsetY;

	/* The number at the end of the original image file name, or -1 if none.
	 * When sprites are packed, if the original file name ends with a number,
	 * it is stored as the index and is not considered as part of the sprite's
	 * name. This is useful for keeping animation frames in order.	 */
	int index;

	/* The ninepatch splits, or null if not a ninepatch. Has 4 elements: left,
	 * right, top, bottom. */
	int* splits;

	/** The ninepatch pads, or null if not a ninepatch or the has no padding.
	 * Has 4 elements: left, right, top, bottom. */
	int* pads;

	/* The regions are organized in a linked list, this links to the next one. */
	struct TextureAtlas_region* nextRegion;
};

struct TextureAtlas_atlas* TextureAtlas_read(const char* filename);

void TextureAtlas_write(struct TextureAtlas_atlas* atlas, const char* filename);

void TextureAtlas_cleanup(struct TextureAtlas_atlas* atlas);

#endif /* TEXTURE_ATLAS_H_ */
