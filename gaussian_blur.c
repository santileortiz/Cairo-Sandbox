//gcc -DGAUSSIAN_BLUR -mssse3 -O3 -g -Wall main.c -o main -lcairo -lX11-xcb -lX11 -lxcb -lxcb-sync -lm

/*
 * Copiright (C) 2018 Santiago León O.
 */

#ifdef GAUSSIAN_BLUR 

#include "emmintrin.h"
#include "pmmintrin.h"

void css_gaussian_blur (cairo_surface_t *image, double r)
{
    assert (cairo_surface_get_type (image) == CAIRO_SURFACE_TYPE_IMAGE);
    assert (cairo_image_surface_get_format (image) == CAIRO_FORMAT_ARGB32);

    if (r == 0) {
        return;
    }

    BEGIN_WALL_CLOCK;
    int size = 2*r+1;
    uint8_t kernel[size];
    int i;
    uint32_t area = 0;
    double mu = ARRAY_SIZE(kernel)/2;
    for (i=0; i<ARRAY_SIZE(kernel); i++) {
        kernel[i] = (uint32_t)(255*exp(-(i-mu)*(i-mu)/(2*(r/2)*(r/2))));
        area += kernel[i];
    }
    PROBE_WALL_CLOCK("Kernel creation");

    uint32_t *src = (uint32_t*)cairo_image_surface_get_data (image);
    int src_width = cairo_image_surface_get_width (image);
    int src_height = cairo_image_surface_get_height (image);
    cairo_surface_t *tmp_surface =
        cairo_image_surface_create (CAIRO_FORMAT_ARGB32, src_width, src_height);
    uint32_t *tmp = (uint32_t*)cairo_image_surface_get_data (tmp_surface);
    PROBE_WALL_CLOCK("Temp surface creation");

    for (i=0; i<src_height; i++) {
        int j;
        for (j=0; j<src_width; j++) {
            int k;
            uint32_t a = 0, r = 0, g = 0, b = 0;
            for (k=0; k<ARRAY_SIZE(kernel); k++) {
                if (j - ARRAY_SIZE(kernel)/2 + k < 0) {
                    continue;
                } else if (j - ARRAY_SIZE(kernel)/2 + k >= src_width) {
                    continue;
                }

                uint8_t *src_px = (uint8_t*)(src + j - ARRAY_SIZE(kernel)/2 + k + i*src_width);
                a += src_px[0]*kernel[k];
                r += src_px[1]*kernel[k];
                g += src_px[2]*kernel[k];
                b += src_px[3]*kernel[k];
            }
            uint8_t *dest_px = (uint8_t*)(tmp + j + i*src_width);
            dest_px[0] = a/area;
            dest_px[1] = r/area;
            dest_px[2] = g/area;
            dest_px[3] = b/area;

        }
    }
    PROBE_WALL_CLOCK("Horizontal blur");

    int j;
    for (j=0; j<src_width; j++) {
        int i;
        for (i=0; i<src_height; i++) {
            int k;
            uint32_t a = 0, r = 0, g = 0, b = 0;
            for (k=0; k<ARRAY_SIZE(kernel); k++) {
                if (i - ARRAY_SIZE(kernel)/2 + k < 0) {
                    continue;
                } else if (i - ARRAY_SIZE(kernel)/2 + k >= src_height) {
                    continue;
                }

                uint8_t *src_px = (uint8_t*)(tmp + j + (i - ARRAY_SIZE(kernel)/2 + k)*src_width);
                a += src_px[0]*kernel[k];
                r += src_px[1]*kernel[k];
                g += src_px[2]*kernel[k];
                b += src_px[3]*kernel[k];
            }
            uint8_t *dest_px = (uint8_t*)(src + j + i*src_width);
            dest_px[0] = a/area;
            dest_px[1] = r/area;
            dest_px[2] = g/area;
            dest_px[3] = b/area;
        }
    }
    PROBE_WALL_CLOCK("Vertical blur");

    cairo_surface_destroy (tmp_surface);
    cairo_surface_mark_dirty (image);
    PROBE_WALL_CLOCK("Cleanup");
}

// This seems to be a bad idea, the only operations we are doing per chanel
// are a multiplication and a division. SIMD does not have integer
// multiplication or division for 8 bits. This forces us to use float and waste
// time in the conversion. We also have to move chanels around a lot wasting
// even more time.
//
// A more sensible approach seems to be implementing IIR gaussian filter
// approximation and then try to SIMD that one as it does not have a kernel and
// the time complexity does not depend on sigma (r). There is also a paper of
// intel describing this implementation.
void css_gaussian_blur_simd (cairo_surface_t *image, double r)
{
    assert (cairo_surface_get_type (image) == CAIRO_SURFACE_TYPE_IMAGE);
    assert (cairo_image_surface_get_format (image) == CAIRO_FORMAT_ARGB32);

    if (r == 0) {
        return;
    }

    BEGIN_WALL_CLOCK;
    int kernel_size = 2*(int)(ceil(r))+1;
    float kernel[kernel_size + (4-kernel_size%4)];
    int i;
    float area = 0;
    double mu = ARRAY_SIZE(kernel)/2;
    for (i=0; i<ARRAY_SIZE(kernel); i++) {
        if (i >= kernel_size) {
            kernel[i] = 0;
        } else {
            kernel[i] = exp(-(i-mu)*(i-mu)/(2*(r/2)*(r/2)));
            area += kernel[i];
        }
    }

    for (i=0; i<ARRAY_SIZE(kernel); i++) {
        kernel[i] /= area;
    }

    //float test = 0;
    //for (i=0; i<ARRAY_SIZE(kernel); i++) {
    //    test += kernel[i];
    //}
    //printf ("Test: %f\n", test);

    printf ("Kernel area: %f\n", area);
    printf ("Kernel size: %d\n", kernel_size);
    printf ("Size: %ld\n", ARRAY_SIZE(kernel));
    PROBE_WALL_CLOCK("Kernel creation");

    uint32_t *src = (uint32_t*)cairo_image_surface_get_data (image);
    int src_width = cairo_image_surface_get_width (image);
    int src_height = cairo_image_surface_get_height (image);
    cairo_surface_t *tmp_surface =
        cairo_image_surface_create (CAIRO_FORMAT_ARGB32, src_width, src_height);
    uint32_t *tmp = (uint32_t*)cairo_image_surface_get_data (tmp_surface);
    PROBE_WALL_CLOCK("Temp surface creation");

    __m128 zero = _mm_set1_ps (0);
    __m128i mask_8bit = _mm_set1_epi32 (0xFF);
    for (i=0; i<src_height; i++) {
        int j;
        for (j=0; j<src_width; j++) {
            int k;

            __m128 dest_px_w = _mm_set1_ps (0);
            for (k=0; k<ARRAY_SIZE(kernel); k+=4) {
                __m128i src_px;
                __m128 kernel_values;
                int K;
                for (K=0; K < 4; K++) {
                    if (j - kernel_size/2 + k + K < 0) {
                        ((float*)&(kernel_values))[K] = 0;
                        continue;
                    } else if (j - kernel_size/2 + k + K >= src_width) {
                        ((float*)&(kernel_values))[K] = 0;
                        continue;
                    } else {
                        ((float*)&(kernel_values))[K] = kernel[k + K];
                        ((uint32_t*)&(src_px))[K] = *(src + j - kernel_size/2 + k + K + i*src_width);
                    }
                }

                __m128 a = _mm_cvtepi32_ps(_mm_and_si128 (_mm_srli_epi32 (src_px, 24), mask_8bit));
                a = _mm_mul_ps (a, kernel_values);
                a = _mm_hadd_ps (zero, a);
                a = _mm_hadd_ps (zero, a);

                __m128 r = _mm_cvtepi32_ps(_mm_and_si128 (_mm_srli_epi32 (src_px, 16), mask_8bit));
                r = _mm_mul_ps (r, kernel_values);
                r = _mm_hadd_ps (r, zero);
                r = _mm_hadd_ps (zero, r);

                __m128 g = _mm_cvtepi32_ps(_mm_and_si128 (_mm_srli_epi32 (src_px, 8), mask_8bit));
                g = _mm_mul_ps (g, kernel_values);
                g = _mm_hadd_ps (zero, g);
                g = _mm_hadd_ps (g, zero);

                __m128 b = _mm_cvtepi32_ps(_mm_and_si128 (src_px, mask_8bit));
                b = _mm_mul_ps (b, kernel_values);
                b = _mm_hadd_ps (b, zero);
                b = _mm_hadd_ps (b, zero);

                dest_px_w = _mm_add_ps (_mm_or_ps (_mm_or_ps (a, r), _mm_or_ps (g, b)), dest_px_w);
            }

            uint32_t res[4];
            __m128i int_dest_px = _mm_cvttps_epi32 (dest_px_w);
            _mm_storeu_si128 ((__m128i*)res, int_dest_px);
            uint8_t *dest_px = (uint8_t*)(tmp + j + i*src_width);
            dest_px[3] = (uint8_t)(res[3]);
            dest_px[2] = (uint8_t)(res[2]);
            dest_px[1] = (uint8_t)(res[1]);
            dest_px[0] = (uint8_t)(res[0]);

            // Why does this warn about aliasing:
            //dest_px[0] = (uint8_t)(((uint32_t*)&(int_dest_px))[0]);
            // But this does not:
            //dest_px[1] = (uint8_t)(((uint32_t*)&(int_dest_px))[1]);

        }
    }
    PROBE_WALL_CLOCK("Horizontal blur");

    int j;
    for (j=0; j<src_width; j++) {
        int i;
        for (i=0; i<src_height; i++) {
            int k;
            float a = 0, r = 0, g = 0, b = 0;
            for (k=0; k<kernel_size; k++) {
                if (i - kernel_size/2 + k < 0) {
                    continue;
                } else if (i - kernel_size/2 + k >= src_height) {
                    continue;
                }

                uint8_t *src_px = (uint8_t*)(tmp + j + (i - kernel_size/2 + k)*src_width);
                a += ((float)src_px[0])*kernel[k];
                r += ((float)src_px[1])*kernel[k];
                g += ((float)src_px[2])*kernel[k];
                b += ((float)src_px[3])*kernel[k];
            }
            uint8_t *dest_px = (uint8_t*)(src + j + i*src_width);
            dest_px[0] = (uint8_t)a;
            dest_px[1] = (uint8_t)r;
            dest_px[2] = (uint8_t)g;
            dest_px[3] = (uint8_t)b;
        }
    }
    PROBE_WALL_CLOCK("Vertical blur");

    cairo_surface_destroy (tmp_surface);
    cairo_surface_mark_dirty (image);
    PROBE_WALL_CLOCK("Cleanup");
}

void pixel_scale (cairo_surface_t *image, int factor)
{
    assert (cairo_surface_get_type (image) == CAIRO_SURFACE_TYPE_IMAGE);
    assert (cairo_image_surface_get_format (image) == CAIRO_FORMAT_ARGB32);

    int image_width = cairo_image_surface_get_width (image)/factor;
    int image_height = cairo_image_surface_get_height (image)/factor;

    cairo_surface_t *tmp = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, image_width, image_height);
    cairo_t *cr_tmp = cairo_create (tmp);
    cairo_set_source_surface (cr_tmp, image, 0, 0);
    cairo_paint (cr_tmp);
    cairo_surface_flush (tmp);
    cairo_surface_flush (image);

    uint32_t *src = (uint32_t*)cairo_image_surface_get_data (tmp);
    uint32_t *dest = (uint32_t*)cairo_image_surface_get_data (image);

    int i;
    for (i=0; i<image_height; i++) {
        int v_repeat;
        for (v_repeat=0; v_repeat<factor; v_repeat++) {
            int j;
            for (j=0; j<image_width; j++) {
                int h_repeat;
                for (h_repeat=0; h_repeat<factor; h_repeat++) {
                    *dest = src[j + i*image_width];
                    dest++;
                }
            }
        }
    }

    cairo_surface_mark_dirty (image);
    cairo_surface_flush (image);
    cairo_surface_destroy (tmp);
}

bool draw (app_graphics_t *gr, bool redraw)
{
    static bool draw_once = true;
    if (draw_once || redraw) {
        draw_once = false;
        cairo_t *cr = gr->cr;
        cairo_set_source_rgb (cr, 1,1,1);
        cairo_paint (cr);

        int r = 4;
        int width = 55;
        int height = 20;
        int x = 5;
        int y = 5;
        cairo_surface_t *image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, gr->width, gr->height);
        cairo_t *img_cr = cairo_create (image);
        cairo_set_source_rgb (img_cr,1,1,1);
        cairo_paint (img_cr);
        cairo_rectangle (img_cr, x, y, 2*r+width, 2*r+height);
        cairo_set_source_rgb (img_cr,1,0,0);
        cairo_fill (img_cr);
        cairo_rectangle (img_cr, x+r, y+r, width, height);
        cairo_set_source_rgb (img_cr,0,1,0);
        cairo_fill (img_cr);
        cairo_move_to (img_cr, x+5, y+18);
        cairo_select_font_face (img_cr, "Open Type", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size (img_cr, 12);
        cairo_set_source_rgb (img_cr,0,0,0);
        cairo_show_text (img_cr, "Arrange");
        cairo_surface_flush (image);

        css_gaussian_blur_simd (image, r);
        pixel_scale (image, 10);

        cairo_set_source_surface (cr, image, 0, 0);
        cairo_paint (cr);

        cairo_surface_flush (cairo_get_target(gr->cr));
        return true;
    } else {
        return false;
    }
}
#endif // GAUSSIAN_BLUR
