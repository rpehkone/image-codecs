static int stbi_write_tga_core(stbi__write_context *s, int x, int y, int comp, void *data)
{
	int has_alpha = (comp == 2 || comp == 4);
	int colorbytes = has_alpha ? comp - 1 : comp;
	int format = colorbytes < 2 ? 3 : 2; // 3 color channels (RGB/RGBA) = 2, 1 color channel (Y/YA) = 3

	if (y < 0 || x < 0)
		return 0;

	if (!stbi_write_tga_with_rle)
	{
		return stbiw__outfile(s, -1, -1, x, y, comp, 0, (void *)data, has_alpha, 0,
									 "111 221 2222 11", 0, 0, format, 0, 0, 0, 0, 0, x, y, (colorbytes + has_alpha) * 8, has_alpha * 8);
	}
	else
	{
		int i, j, k;
		int jend, jdir;

		stbiw__writef(s, "111 221 2222 11", 0, 0, format + 8, 0, 0, 0, 0, 0, x, y, (colorbytes + has_alpha) * 8, has_alpha * 8);

		if (stbi__flip_vertically_on_write)
		{
			j = 0;
			jend = y;
			jdir = 1;
		}
		else
		{
			j = y - 1;
			jend = -1;
			jdir = -1;
		}
		for (; j != jend; j += jdir)
		{
			unsigned char *row = (unsigned char *)data + j * x * comp;
			int len;

			for (i = 0; i < x; i += len)
			{
				unsigned char *begin = row + i * comp;
				int diff = 1;
				len = 1;

				if (i < x - 1)
				{
					++len;
					diff = memcmp(begin, row + (i + 1) * comp, comp);
					if (diff)
					{
						const unsigned char *prev = begin;
						for (k = i + 2; k < x && len < 128; ++k)
						{
							if (memcmp(prev, row + k * comp, comp))
							{
								prev += comp;
								++len;
							}
							else
							{
								--len;
								break;
							}
						}
					}
					else
					{
						for (k = i + 2; k < x && len < 128; ++k)
						{
							if (!memcmp(begin, row + k * comp, comp))
							{
								++len;
							}
							else
							{
								break;
							}
						}
					}
				}

				if (diff)
				{
					unsigned char header = STBIW_UCHAR(len - 1);
					stbiw__write1(s, header);
					for (k = 0; k < len; ++k)
					{
						stbiw__write_pixel(s, -1, comp, has_alpha, 0, begin + k * comp);
					}
				}
				else
				{
					unsigned char header = STBIW_UCHAR(len - 129);
					stbiw__write1(s, header);
					stbiw__write_pixel(s, -1, comp, has_alpha, 0, begin);
				}
			}
		}
		stbiw__write_flush(s);
	}
	return 1;
}

STBIWDEF int stbi_write_tga_to_func(stbi_write_func *func, void *context, int x, int y, int comp, const void *data)
{
	stbi__write_context s = {0};
	stbi__start_write_callbacks(&s, func, context);
	return stbi_write_tga_core(&s, x, y, comp, (void *)data);
}

#ifndef STBI_WRITE_NO_STDIO
STBIWDEF int stbi_write_tga(char const *filename, int x, int y, int comp, const void *data)
{
	stbi__write_context s = {0};
	if (stbi__start_write_file(&s, filename))
	{
		int r = stbi_write_tga_core(&s, x, y, comp, (void *)data);
		stbi__end_write_file(&s);
		return r;
	}
	else
		return 0;
}
#endif
