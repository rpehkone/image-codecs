#define stbiw__max(a, b) ((a) > (b) ? (a) : (b))

#ifndef STBI_WRITE_NO_STDIO

static void stbiw__linear_to_rgbe(unsigned char *rgbe, float *linear)
{
	int exponent;
	float maxcomp = stbiw__max(linear[0], stbiw__max(linear[1], linear[2]));

	if (maxcomp < 1e-32f)
	{
		rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
	}
	else
	{
		float normalize = (float)frexp(maxcomp, &exponent) * 256.0f / maxcomp;

		rgbe[0] = (unsigned char)(linear[0] * normalize);
		rgbe[1] = (unsigned char)(linear[1] * normalize);
		rgbe[2] = (unsigned char)(linear[2] * normalize);
		rgbe[3] = (unsigned char)(exponent + 128);
	}
}

static void stbiw__write_run_data(stbi__write_context *s, int length, unsigned char databyte)
{
	unsigned char lengthbyte = STBIW_UCHAR(length + 128);
	STBIW_ASSERT(length + 128 <= 255);
	s->func(s->context, &lengthbyte, 1);
	s->func(s->context, &databyte, 1);
}

static void stbiw__write_dump_data(stbi__write_context *s, int length, unsigned char *data)
{
	unsigned char lengthbyte = STBIW_UCHAR(length);
	STBIW_ASSERT(length <= 128); // inconsistent with spec but consistent with official code
	s->func(s->context, &lengthbyte, 1);
	s->func(s->context, data, length);
}

static void stbiw__write_hdr_scanline(stbi__write_context *s, int width, int ncomp, unsigned char *scratch, float *scanline)
{
	unsigned char scanlineheader[4] = {2, 2, 0, 0};
	unsigned char rgbe[4];
	float linear[3];
	int x;

	scanlineheader[2] = (width & 0xff00) >> 8;
	scanlineheader[3] = (width & 0x00ff);

	/* skip RLE for images too small or large */
	if (width < 8 || width >= 32768)
	{
		for (x = 0; x < width; x++)
		{
			switch (ncomp)
			{
			case 4: /* fallthrough */
			case 3:
				linear[2] = scanline[x * ncomp + 2];
				linear[1] = scanline[x * ncomp + 1];
				linear[0] = scanline[x * ncomp + 0];
				break;
			default:
				linear[0] = linear[1] = linear[2] = scanline[x * ncomp + 0];
				break;
			}
			stbiw__linear_to_rgbe(rgbe, linear);
			s->func(s->context, rgbe, 4);
		}
	}
	else
	{
		int c, r;
		/* encode into scratch buffer */
		for (x = 0; x < width; x++)
		{
			switch (ncomp)
			{
			case 4: /* fallthrough */
			case 3:
				linear[2] = scanline[x * ncomp + 2];
				linear[1] = scanline[x * ncomp + 1];
				linear[0] = scanline[x * ncomp + 0];
				break;
			default:
				linear[0] = linear[1] = linear[2] = scanline[x * ncomp + 0];
				break;
			}
			stbiw__linear_to_rgbe(rgbe, linear);
			scratch[x + width * 0] = rgbe[0];
			scratch[x + width * 1] = rgbe[1];
			scratch[x + width * 2] = rgbe[2];
			scratch[x + width * 3] = rgbe[3];
		}

		s->func(s->context, scanlineheader, 4);

		/* RLE each component separately */
		for (c = 0; c < 4; c++)
		{
			unsigned char *comp = &scratch[width * c];

			x = 0;
			while (x < width)
			{
				// find first run
				r = x;
				while (r + 2 < width)
				{
					if (comp[r] == comp[r + 1] && comp[r] == comp[r + 2])
						break;
					++r;
				}
				if (r + 2 >= width)
					r = width;
				// dump up to first run
				while (x < r)
				{
					int len = r - x;
					if (len > 128)
						len = 128;
					stbiw__write_dump_data(s, len, &comp[x]);
					x += len;
				}
				// if there's a run, output it
				if (r + 2 < width)
				{ // same test as what we break out of in search loop, so only true if we break'd
					// find next byte after run
					while (r < width && comp[r] == comp[x])
						++r;
					// output run up to r
					while (x < r)
					{
						int len = r - x;
						if (len > 127)
							len = 127;
						stbiw__write_run_data(s, len, comp[x]);
						x += len;
					}
				}
			}
		}
	}
}

static int stbi_write_hdr_core(stbi__write_context *s, int x, int y, int comp, float *data)
{
	if (y <= 0 || x <= 0 || data == NULL)
		return 0;
	else
	{
		// Each component is stored separately. Allocate scratch space for full output scanline.
		unsigned char *scratch = (unsigned char *)STBIW_MALLOC(x * 4);
		int i, len;
		char buffer[128];
		char header[] = "#?RADIANCE\n# Written by stb_image_write.h\nFORMAT=32-bit_rle_rgbe\n";
		s->func(s->context, header, sizeof(header) - 1);

#ifdef __STDC_LIB_EXT1__
		len = sprintf_s(buffer, sizeof(buffer), "EXPOSURE=          1.0000000000000\n\n-Y %d +X %d\n", y, x);
#else
		len = sprintf(buffer, "EXPOSURE=          1.0000000000000\n\n-Y %d +X %d\n", y, x);
#endif
		s->func(s->context, buffer, len);

		for (i = 0; i < y; i++)
			stbiw__write_hdr_scanline(s, x, comp, scratch, data + comp * x * (stbi__flip_vertically_on_write ? y - 1 - i : i));
		STBIW_FREE(scratch);
		return 1;
	}
}

STBIWDEF int stbi_write_hdr_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const float *data)
{
	stbi__write_context s = {0};
	stbi__start_write_callbacks(&s, func, context);
	return stbi_write_hdr_core(&s, x, y, comp, (float *)data);
}

STBIWDEF int stbi_write_hdr(char const *filename, int x, int y, int comp, const float *data)
{
	stbi__write_context s = {0};
	if (stbi__start_write_file(&s, filename))
	{
		int r = stbi_write_hdr_core(&s, x, y, comp, (float *)data);
		stbi__end_write_file(&s);
		return r;
	}
	else
		return 0;
}
#endif // STBI_WRITE_NO_STDIO
