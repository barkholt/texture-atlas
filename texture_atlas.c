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

#include "texture_atlas.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <libgen.h>

#define BUFFER_SIZE 1024
static const char* FORMAT_RGBA8888 = "RGBA8888";
static const char* FORMAT_RGB888 = "RGB888";
static const char* FORMAT_RGBA4444 = "RGBA4444";
static const char* FORMAT_RGB565 = "RGB565";
static const char* FORMAT_LUMINANCE_ALPHA = "LuminanceAlpha";
static const char* FORMAT_INTENSITY = "Intensity";
static const char* FORMAT_ALPHA = "Alpha";

static const char* REPEAT_NONE = "none";
static const char* REPEAT_XY = "xy";
static const char* REPEAT_Y = "y";
static const char* REPEAT_X = "x";

static const char* FILTER_MIP_MAP_LINEAR_LINEAR = "MipMapLinearLinear";
static const char* FILTER_MIP_MAP_NEAREST_LINEAR = "MipMapNearestLinear";
static const char* FILTER_MIP_MAP_LINEAR_NEAREST = "MipMapLinearNearest";
static const char* FILTER_MIP_MAP_NEAREST_NEAREST = "MipMapNearestNearest";
static const char* FILTER_MIP_MAP = "MipMap";
static const char* FILTER_LINEAR = "Linear";
static const char* FILTER_NEAREST = "Nearest";

const char* filterEnumToString(enum TextureAtlas_filter filter) {
	if (TextureAtlas_NEAREST == filter) {
		return FILTER_NEAREST;
	} else if (TextureAtlas_LINEAR == filter) {
		return FILTER_LINEAR;
	} else if (TextureAtlas_MIP_MAP == filter) {
		return FILTER_MIP_MAP;
	} else if (TextureAtlas_MIP_MAP_NEAREST_NEAREST == filter) {
		return FILTER_MIP_MAP_NEAREST_NEAREST;
	} else if (TextureAtlas_MIP_MAP_LINEAR_NEAREST == filter) {
		return FILTER_MIP_MAP_LINEAR_NEAREST;
	} else if (TextureAtlas_MIP_MAP_NEAREST_LINEAR == filter) {
		return FILTER_MIP_MAP_NEAREST_LINEAR;
	} else if (TextureAtlas_MIP_MAP_LINEAR_LINEAR == filter) {
		return FILTER_MIP_MAP_LINEAR_LINEAR;
	} else
		return "ERROR!!!!";
}

const char* repeatEnumToString(enum TextureAtlas_repeat repeat) {
	if (repeat == TextureAtlas_X) {
		return REPEAT_X;
	} else if (repeat == TextureAtlas_Y) {
		return REPEAT_Y;
	} else if (repeat == TextureAtlas_XY) {
		return REPEAT_XY;
	} else if (repeat == TextureAtlas_NONE) {
		return REPEAT_NONE;
	} else {
		return "ERROR!!!!";
	}
}

const char* formatEnumToString(enum TextureAtlas_format format) {
	if (format == TextureAtlas_ALPHA)
		return FORMAT_ALPHA;
	else if (format == TextureAtlas_INTENSITY)
		return FORMAT_INTENSITY;
	else if (format == TextureAtlas_LUMINANCE_ALPHA)
		return FORMAT_LUMINANCE_ALPHA;
	else if (format == TextureAtlas_RGB565)
		return FORMAT_RGB565;
	else if (format == TextureAtlas_RGBA4444)
		return FORMAT_RGBA4444;
	else if (format == TextureAtlas_RGB888)
		return FORMAT_RGB888;
	else if (format == TextureAtlas_RGBA8888)
		return FORMAT_RGBA8888;
	else
		return "ERROR!!!!";
}

bool parse_filter_value(char* value, enum TextureAtlas_filter* result) {
	if (strcmp(value, FILTER_NEAREST) == 0) {
		*result = TextureAtlas_NEAREST;
	} else if (strcmp(value, FILTER_LINEAR) == 0) {
		*result = TextureAtlas_LINEAR;
	} else if (strcmp(value, FILTER_MIP_MAP) == 0) {
		*result = TextureAtlas_MIP_MAP;
	} else if (strcmp(value, FILTER_MIP_MAP_NEAREST_NEAREST) == 0) {
		*result = TextureAtlas_MIP_MAP_NEAREST_NEAREST;
	} else if (strcmp(value, FILTER_MIP_MAP_LINEAR_NEAREST) == 0) {
		*result = TextureAtlas_MIP_MAP_LINEAR_NEAREST;
	} else if (strcmp(value, FILTER_MIP_MAP_NEAREST_LINEAR) == 0) {
		*result = TextureAtlas_MIP_MAP_NEAREST_LINEAR;
	} else if (strcmp(value, FILTER_MIP_MAP_LINEAR_LINEAR) == 0) {
		*result = TextureAtlas_MIP_MAP_LINEAR_LINEAR;
	} else {
		fprintf(stderr, "ERROR. TextureAtlas: Unknown 'filter' token value: '%s'\n", value);
		return false;
	}
	return true;
}

void* display_error(TextureAtlas_atlas* atlas, char* message, ...) {
	va_list argptr;
	va_start(argptr, message);
	vfprintf(stderr, message, argptr);
	va_end(argptr);

	if (atlas != NULL)
		TextureAtlas_cleanup(atlas);

	return NULL;
}

char* read_name(ssize_t charactersRead, char* lineBuffer) {
	if (charactersRead <= 0)
		return NULL;

	if (lineBuffer[charactersRead - 1] == '\n') {
		charactersRead -= 1;
	}

	if (charactersRead == 0)
		return NULL;

	char* name = strndup(lineBuffer, charactersRead);
	return name;
}

bool is_new_page(ssize_t charactersRead, char* lineBuffer) {
	return charactersRead == 1 && lineBuffer[0] == '\n';
}

bool parse_attribute(ssize_t charactersRead, char* lineBuffer, char* attribute, char* value, int leadBlanks) {

	// If we have read less than the required blanks, the line is certainly invalid
	if (charactersRead < leadBlanks)
		return false;

	// Test if those lead blanks are actually there
	if (leadBlanks > 0) {
		for (int i = 0; i < leadBlanks; i++)
			if (lineBuffer[i] != ' ')
				return false;

		// Advance past the blanks
		lineBuffer += leadBlanks;

		// And make sure we subtract the characters skipped from the total
		charactersRead -= leadBlanks;
	}

	// Are there any characters left for us to handle?
	if (charactersRead <= 0)
		return false;

	// We don't want to look at any ending newline
	if (lineBuffer[charactersRead - 1] == '\n') {
		charactersRead -= 1;
	}

	// Are there STILL any characters for us to look at?
	if (charactersRead == 0)
		return false;

	// Now make sure there is no leading whitespace
	if (isspace(lineBuffer[0]))
		return false;

	char* tokenPos = strchr(lineBuffer, ':');
	if (tokenPos == NULL)
		return false;

	int numSeparatingCharacters = 2;

	// +1 because snprintf counts the terminating 0 as part of the characters to copy
	int attributeLength = tokenPos - lineBuffer + 1;
	int valueLength = charactersRead - numSeparatingCharacters - attributeLength + 2;

	snprintf(attribute, attributeLength, "%s", lineBuffer);
	snprintf(value, valueLength, "%s", tokenPos + numSeparatingCharacters);

	return true;
}

void freeRegion(TextureAtlas_region* region) {
	if (region == NULL)
		return;

	if (region->pads != NULL)
		free(region->pads);

	if (region->splits != NULL)
		free(region->splits);

	if (region->name != NULL)
		free(region->name);

	TextureAtlas_region* next = region->nextRegion;

	free(region);

	// Recursively free the next in the list. Tail recursion for optimization
	freeRegion(next);
}

void freePage(TextureAtlas_page* page) {
	if (page == NULL)
		return;

	if (page->firstRegion != NULL) {
		freeRegion(page->firstRegion);
	}

	TextureAtlas_page* next = page->next;

	if (page->name != NULL)
		free(page->name);

	if (page->absolutePath != NULL)
		free(page->absolutePath);

	free(page);

	freePage(next);
}

void TextureAtlas_write(TextureAtlas_atlas* atlas, const char* filename) {
	FILE* destination = fopen(filename, "w");

	if (destination == NULL)
		return;

	TextureAtlas_page* nextPage = atlas->firstPage;
	while (nextPage != NULL) {
		fprintf(destination, "\n");
		fprintf(destination, "%s\n", nextPage->name);
		fprintf(destination, "size: %i,%i\n", nextPage->width, nextPage->height);
		fprintf(destination, "format: %s\n", formatEnumToString(nextPage->format));
		fprintf(destination, "filter: %s,%s\n", filterEnumToString(nextPage->minificationFilter), filterEnumToString(nextPage->magnificationFilter));
		fprintf(destination, "repeat: %s\n", repeatEnumToString(nextPage->repeat));

		TextureAtlas_region* region = nextPage->firstRegion;

		while (region != NULL) {
			fprintf(destination, "%s\n", region->name);
			fprintf(destination, "  rotate: %s\n", region->rotate ? "true" : "false");
			fprintf(destination, "  xy: %i, %i\n", region->x, region->y);
			fprintf(destination, "  size: %i, %i\n", region->width, region->height);
			if (region->splits != NULL)
				fprintf(destination, "  split: %i, %i, %i, %i\n", region->splits[0], region->splits[1], region->splits[2], region->splits[3]);
			if (region->pads != NULL)
				fprintf(destination, "  pad: %i, %i, %i, %i\n", region->pads[0], region->pads[1], region->pads[2], region->pads[3]);
			fprintf(destination, "  orig: %i, %i\n", region->originalWidth, region->originalHeight);
			fprintf(destination, "  offset: %i, %i\n", region->offsetX, region->offsetY);
			fprintf(destination, "  index: %i\n", region->index);

			region = region->nextRegion;
		}
		nextPage = nextPage->next;
	}

	fclose(destination);
}

void TextureAtlas_cleanup(TextureAtlas_atlas* atlas) {

	if (atlas != NULL) {

		freePage(atlas->firstPage);

		free(atlas);
	}
}

TextureAtlas_atlas* TextureAtlas_read(const char* filename) {

	FILE* atlasFile = fopen(filename, "r");

	// If we could not open the file, return a NULL pointer and let the caller deal with it.
	if (atlasFile == NULL)
		return NULL;

	// Create and initialize the atlas object we will be injecting data into
	TextureAtlas_atlas* atlas = malloc(sizeof(TextureAtlas_atlas));
	atlas->firstPage = NULL;
	atlas->numberOfPages = 0;

	// Setup variables for reading lines
	char* lineBuffer = calloc(BUFFER_SIZE, sizeof(char));
	size_t bufferSize = 0;
	ssize_t charactersRead = 0;

	charactersRead = getline(&lineBuffer, &bufferSize, atlasFile);

	if (!is_new_page(charactersRead, lineBuffer)) {
		return display_error(atlas, "ERROR. TextureAtlas: Expected atlas file to start with newline: '%s'.", filename);
	}

	// Keep track of the previous page, to allow creation of linked list of pages.
	TextureAtlas_page* previousPage = NULL;

	bool keepScanningPages = true;
	int nextPageIndex = 0;
	while (keepScanningPages) {
		// Create page, and initialized to a known invalid state
		TextureAtlas_page* page = malloc(sizeof(TextureAtlas_page));
		page->index = nextPageIndex++;
		page->name = NULL;
		page->next = NULL;
		page->absolutePath = NULL;
		page->width = page->height = -1;
		page->format = TextureAtlas_UNDEFINED_FORMAT;
		page->repeat = TextureAtlas_UNDEFINED_REPEAT;
		page->minificationFilter = TextureAtlas_UNDEFINED_FILTER;
		page->magnificationFilter = TextureAtlas_UNDEFINED_FILTER;

		// Add the page to the atlas now, so we can free() it in case of an error triggering a cleanup
		if (atlas->firstPage == NULL) {
			atlas->firstPage = page;
		} else {
			previousPage->next = page;
		}

		// Record another page added
		atlas->numberOfPages++;

		// Attempt to read the page name
		charactersRead = getline(&lineBuffer, &bufferSize, atlasFile);
		char* pageName = read_name(charactersRead, lineBuffer);
		if (pageName == NULL)
			return display_error(atlas, "ERROR. TextureAtlas: Could not find page name in file '%s'.", filename);
		page->name = pageName;

		// Compute the absolute path to the page image
		char* absPathToAtlas = realpath(filename, NULL);
		char* absPathToDir = dirname(absPathToAtlas);
		asprintf(&page->absolutePath, "%s/%s", absPathToDir, page->name);
		free(absPathToAtlas);

		while ((charactersRead = getline(&lineBuffer, &bufferSize, atlasFile)) > 0) {
			char attribute[BUFFER_SIZE];
			char value[BUFFER_SIZE];

			bool success = parse_attribute(charactersRead, lineBuffer, attribute, value, 0);

			if (!success)
				break;

			if (strcmp(attribute, "size") == 0) {
				unsigned int width, height;
				if (sscanf(value, "%u, %u", &width, &height) == 2) {
					page->width = width;
					page->height = width;
				} else {
					return display_error(atlas, "ERROR. TextureAtlas: Could not read two size tokens: '%s'\n", value);
				}
			} else if (strcmp(attribute, "format") == 0) {
				if (strcmp(value, FORMAT_ALPHA) == 0) {
					page->format = TextureAtlas_ALPHA;
				} else if (strcmp(value, FORMAT_INTENSITY) == 0) {
					page->format = TextureAtlas_INTENSITY;
				} else if (strcmp(value, FORMAT_LUMINANCE_ALPHA) == 0) {
					page->format = TextureAtlas_LUMINANCE_ALPHA;
				} else if (strcmp(value, FORMAT_RGB565) == 0) {
					page->format = TextureAtlas_RGB565;
				} else if (strcmp(value, FORMAT_RGBA4444) == 0) {
					page->format = TextureAtlas_RGBA4444;
				} else if (strcmp(value, FORMAT_RGB888) == 0) {
					page->format = TextureAtlas_RGB888;
				} else if (strcmp(value, FORMAT_RGBA8888) == 0) {
					page->format = TextureAtlas_RGBA8888;
				} else {
					return display_error(atlas, "ERROR. TextureAtlas: Unknown 'format' value: '%s'\n", value);
				}
			} else if (strcmp(attribute, "filter") == 0) {

				/* Get the texture minification and magnification filters */
				char* separator = strchr(value, ',');
				if (separator != NULL) {
					*separator = 0;
					char* firstValue = value;
					char* lastValue = separator + 1;

					if (isspace(*lastValue))
						lastValue++;

					int lastLength = strlen(lastValue);
					if (lastValue[lastLength - 1] == '\n') {
						lastValue[lastLength - 1] = 0;
					}

					enum TextureAtlas_filter minFilter;
					enum TextureAtlas_filter magFilter;
					bool success = parse_filter_value(firstValue, &minFilter);
					success = success && parse_filter_value(lastValue, &magFilter);

					if (!success) {
						TextureAtlas_cleanup(atlas);
						return NULL;
					}
					page->minificationFilter = minFilter;
					page->magnificationFilter = magFilter;
				} else {
					return display_error(atlas, "ERROR. TextureAtlas: Could not read two filter tokens: '%s'\n", value);
				}
			} else if (strcmp(attribute, "repeat") == 0) {
				if (strcmp(value, REPEAT_X) == 0) {
					page->repeat = TextureAtlas_X;
				} else if (strcmp(value, REPEAT_Y) == 0) {
					page->repeat = TextureAtlas_Y;
				} else if (strcmp(value, REPEAT_XY) == 0) {
					page->repeat = TextureAtlas_XY;
				} else if (strcmp(value, REPEAT_NONE) == 0) {
					page->repeat = TextureAtlas_NONE;
				} else {
					return display_error(atlas, "ERROR. TextureAtlas: Unknown 'repeat' value: '%s'\n", value);
				}
			}
		}

		// Check that every field in the page has been correctly initialized
		if (page->name == NULL)
			return display_error(atlas, "'name' value not set in TextureAtlas page in file: '%s'\n", filename);
		if (page->width == -1 || page->height == -1)
			return display_error(atlas, "'size' value not properly set in TextureAtlas page '%s' in file: '%s'.\n", page->name, filename);
		if (page->format == TextureAtlas_UNDEFINED_FORMAT)
			return display_error(atlas, "'format' value not properly set in TextureAtlas page '%s' in file: '%s'.\n", page->name, filename);
		if (page->repeat == TextureAtlas_UNDEFINED_REPEAT)
			return display_error(atlas, "'repeat' value not properly set in TextureAtlas page '%s' in file: '%s'.\n", page->name, filename);
		if (page->minificationFilter == TextureAtlas_UNDEFINED_FILTER || page->magnificationFilter == TextureAtlas_UNDEFINED_FILTER)
			return display_error(atlas, "'filter' value not properly set in TextureAtlas page '%s' in file: '%s'.\n", page->name, filename);

		previousPage = page;

		TextureAtlas_region* previousRegion = NULL;

		// Start scanning regions
		bool keepScanningRegions = true;
		while (keepScanningRegions) {
			// Create region to fill, and initialize to known default/invalid state.
			TextureAtlas_region* region = malloc(sizeof(TextureAtlas_region));
			region->page = page;
			region->name = NULL;
			region->width = -1;
			region->height = -1;
			region->index = -1;
			region->offsetX = -1;
			region->offsetY = -1;
			region->originalHeight = -1;
			region->originalWidth = -1;
			region->pads = NULL;
			region->rotate = false;
			region->splits = NULL;
			region->x = -1;
			region->y = -1;
			region->nextRegion = NULL;

			// Setup this now, so it will be cleanup on an error
			if (previousRegion == NULL) {
				page->firstRegion = region;
			} else {
				previousRegion->nextRegion = region;
			}

			// Attempt to read the page name
			char* regionName = read_name(charactersRead, lineBuffer);

			if (regionName == NULL)
				return display_error(atlas, "ERROR. TextureAtlas: Expected region name in file '%s'.", filename);

			region->name = regionName;

			while ((charactersRead = getline(&lineBuffer, &bufferSize, atlasFile)) > 0) {

				char attribute[BUFFER_SIZE];
				char value[BUFFER_SIZE];
				bool success = parse_attribute(charactersRead, lineBuffer, attribute, value, 2);
				if (!success)
					break;

				if (strcmp(attribute, "rotate") == 0) {
					if (strcmp(value, "false") == 0) {
						region->rotate = false;
					} else if (strcmp(value, "true") == 0) {
						region->rotate = true;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Unknown value in 'rotate' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, REPEAT_XY) == 0) {
					unsigned int x, y;
					if (sscanf(value, "%u, %u", &x, &y) == 2) {
						region->x = x;
						region->y = y;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'xy' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, "size") == 0) {
					unsigned int width, height;
					if (sscanf(value, "%u, %u", &width, &height) == 2) {
						region->width = width;
						region->height = height;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'size' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, "orig") == 0) {
					unsigned int width, height;
					if (sscanf(value, "%u, %u", &width, &height) == 2) {
						region->originalWidth = width;
						region->originalHeight = height;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'orig' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, "offset") == 0) {
					unsigned int offsetX, offsetY;
					if (sscanf(value, "%u, %u", &offsetX, &offsetY) == 2) {
						region->offsetX = offsetX;
						region->offsetY = offsetY;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'offset' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, "index") == 0) {
					unsigned int index;
					if (sscanf(value, "%u", &index) == 1) {
						region->index = index;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'index' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, "split") == 0) {
					unsigned int v1, v2, v3, v4;
					if (sscanf(value, "%u, %u, %u, %u", &v1, &v2, &v3, &v4) == 4) {
						int* splits = malloc(sizeof(int) * 4);
						splits[0] = v1;
						splits[1] = v2;
						splits[2] = v3;
						splits[3] = v4;
						region->splits = splits;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'split' token: '%s'\n", value);
					}
				} else if (strcmp(attribute, "pad") == 0) {
					unsigned int v1, v2, v3, v4;
					if (sscanf(value, "%u, %u, %u, %u", &v1, &v2, &v3, &v4) == 4) {
						int* pads = malloc(sizeof(int) * 4);
						pads[0] = v1;
						pads[1] = v2;
						pads[2] = v3;
						pads[3] = v4;
						region->pads = pads;
					} else {
						return display_error(atlas, "ERROR. TextureAtlas: Could not read 'pad' token: '%s'\n", value);
					}
				}
			}

			// No more attributes - what did we hit instead?
			if (charactersRead == 0 || charactersRead == -1) {
				// We are completly done
				keepScanningRegions = false;
				keepScanningPages = false;
			} else if (is_new_page(charactersRead, lineBuffer)) {
				keepScanningRegions = false;
			} else {
				// Should be the beginning of a new region then, so let the loop continue
			}

			previousRegion = region;
		}

	}
	/* Deallocate temporary memory */
	free(lineBuffer);

	return atlas;
}

TextureAtlas_region* TextureAtlas_findRegion(TextureAtlas_atlas* atlas, char* regionName) {
	TextureAtlas_page* currentPage = atlas->firstPage;
	while (currentPage != NULL) {
		TextureAtlas_region* currentRegion = currentPage->firstRegion;
		while (currentRegion != NULL) {
			if (strcmp(currentRegion->name, regionName) == 0) {
				return currentRegion;
			}
			currentRegion = currentRegion->nextRegion;
		}
		currentPage = currentPage->next;
	}
	return NULL;
}

