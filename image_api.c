

static void *stbi__load_main(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri, int bpc)
{
	memset(ri, 0, sizeof(*ri));         // make sure it's initialized if we add new fields
	ri->bits_per_channel = 8;           // default is 8 so most paths don't have to be changed
	ri->channel_order = STBI_ORDER_RGB; // all current input & output are this, but this is here so we can add BGR order
	ri->num_channels = 0;

#ifndef STBI_NO_JPEG
	if (stbi__jpeg_test(s))
		return stbi__jpeg_load(s, x, y, comp, req_comp, ri);
#endif
#ifndef STBI_NO_PNG
	if (stbi__png_test(s))
		return stbi__png_load(s, x, y, comp, req_comp, ri);
#endif
#ifndef STBI_NO_BMP
	if (stbi__bmp_test(s))
		return stbi__bmp_load(s, x, y, comp, req_comp, ri);
#endif
#ifndef STBI_NO_GIF
	if (stbi__gif_test(s))
		return stbi__gif_load(s, x, y, comp, req_comp, ri);
#endif
#ifndef STBI_NO_PSD
	if (stbi__psd_test(s))
		return stbi__psd_load(s, x, y, comp, req_comp, ri, bpc);
#else
	STBI_NOTUSED(bpc);
#endif
#ifndef STBI_NO_PIC
	if (stbi__pic_test(s))
		return stbi__pic_load(s, x, y, comp, req_comp, ri);
#endif
#ifndef STBI_NO_PNM
	if (stbi__pnm_test(s))
		return stbi__pnm_load(s, x, y, comp, req_comp, ri);
#endif

#ifndef STBI_NO_HDR
	if (stbi__hdr_test(s))
	{
		float *hdr = stbi__hdr_load(s, x, y, comp, req_comp, ri);
		return stbi__hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
	}
#endif

#ifndef STBI_NO_TGA
	// test tga last because it's a crappy test!
	if (stbi__tga_test(s))
		return stbi__tga_load(s, x, y, comp, req_comp, ri);
#endif

	return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
}

static int stbi__is_16_main(stbi__context *s)
{
#ifndef STBI_NO_PNG
	if (stbi__png_is16(s))
		return 1;
#endif

#ifndef STBI_NO_PSD
	if (stbi__psd_is16(s))
		return 1;
#endif

	return 0;
}

#ifndef STBI_NO_STDIO
STBIDEF int stbi_info(char const *filename, int *x, int *y, int *comp)
{
	FILE *f = stbi__fopen(filename, "rb");
	int result;
	if (!f)
		return stbi__err("can't fopen", "Unable to open file");
	result = stbi_info_from_file(f, x, y, comp);
	fclose(f);
	return result;
}

STBIDEF int stbi_info_from_file(FILE *f, int *x, int *y, int *comp)
{
	int r;
	stbi__context s;
	long pos = ftell(f);
	stbi__start_file(&s, f);
	r = stbi__info_main(&s, x, y, comp);
	fseek(f, pos, SEEK_SET);
	return r;
}

STBIDEF int stbi_is_16_bit(char const *filename)
{
	FILE *f = stbi__fopen(filename, "rb");
	int result;
	if (!f)
		return stbi__err("can't fopen", "Unable to open file");
	result = stbi_is_16_bit_from_file(f);
	fclose(f);
	return result;
}

STBIDEF int stbi_is_16_bit_from_file(FILE *f)
{
	int r;
	stbi__context s;
	long pos = ftell(f);
	stbi__start_file(&s, f);
	r = stbi__is_16_main(&s);
	fseek(f, pos, SEEK_SET);
	return r;
}
#endif // !STBI_NO_STDIO

STBIDEF int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp)
{
	stbi__context s;
	stbi__start_mem(&s, buffer, len);
	return stbi__info_main(&s, x, y, comp);
}

STBIDEF int stbi_info_from_callbacks(stbi_io_callbacks const *c, void *user, int *x, int *y, int *comp)
{
	stbi__context s;
	stbi__start_callbacks(&s, (stbi_io_callbacks *)c, user);
	return stbi__info_main(&s, x, y, comp);
}

STBIDEF int stbi_is_16_bit_from_memory(stbi_uc const *buffer, int len)
{
	stbi__context s;
	stbi__start_mem(&s, buffer, len);
	return stbi__is_16_main(&s);
}

STBIDEF int stbi_is_16_bit_from_callbacks(stbi_io_callbacks const *c, void *user)
{
	stbi__context s;
	stbi__start_callbacks(&s, (stbi_io_callbacks *)c, user);
	return stbi__is_16_main(&s);
}
