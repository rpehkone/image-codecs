// Known limitations:
//   Does not support comments in the header section
//   Does not support ASCII image data (formats P2 and P3)
//   Does not support 16-bit-per-channel

#ifndef STBI_NO_PNM

static int stbi__pnm_test(stbi__context *s)
{
	char p, t;
	p = (char)stbi__get8(s);
	t = (char)stbi__get8(s);
	if (p != 'P' || (t != '5' && t != '6'))
	{
		stbi__rewind(s);
		return 0;
	}
	return 1;
}

static void *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp, stbi__result_info *ri)
{
	stbi_uc *out;
	STBI_NOTUSED(ri);

	if (!stbi__pnm_info(s, (int *)&s->img_x, (int *)&s->img_y, (int *)&s->img_n))
		return 0;

	if (s->img_y > STBI_MAX_DIMENSIONS)
		return stbi__errpuc("too large", "Very large image (corrupt?)");
	if (s->img_x > STBI_MAX_DIMENSIONS)
		return stbi__errpuc("too large", "Very large image (corrupt?)");

	*x = s->img_x;
	*y = s->img_y;
	if (comp)
		*comp = s->img_n;

	if (!stbi__mad3sizes_valid(s->img_n, s->img_x, s->img_y, 0))
		return stbi__errpuc("too large", "PNM too large");

	out = (stbi_uc *)stbi__malloc_mad3(s->img_n, s->img_x, s->img_y, 0);
	if (!out)
		return stbi__errpuc("outofmem", "Out of memory");
	stbi__getn(s, out, s->img_n * s->img_x * s->img_y);

	if (req_comp && req_comp != s->img_n)
	{
		out = stbi__convert_format(out, s->img_n, req_comp, s->img_x, s->img_y);
		if (out == NULL)
			return out; // stbi__convert_format frees input on failure
	}
	return out;
}

static int stbi__pnm_isspace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

static void stbi__pnm_skip_whitespace(stbi__context *s, char *c)
{
	for (;;)
	{
		while (!stbi__at_eof(s) && stbi__pnm_isspace(*c))
			*c = (char)stbi__get8(s);

		if (stbi__at_eof(s) || *c != '#')
			break;

		while (!stbi__at_eof(s) && *c != '\n' && *c != '\r')
			*c = (char)stbi__get8(s);
	}
}

static int stbi__pnm_isdigit(char c)
{
	return c >= '0' && c <= '9';
}

static int stbi__pnm_getinteger(stbi__context *s, char *c)
{
	int value = 0;

	while (!stbi__at_eof(s) && stbi__pnm_isdigit(*c))
	{
		value = value * 10 + (*c - '0');
		*c = (char)stbi__get8(s);
	}

	return value;
}

static int stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp)
{
	int maxv, dummy;
	char c, p, t;

	if (!x)
		x = &dummy;
	if (!y)
		y = &dummy;
	if (!comp)
		comp = &dummy;

	stbi__rewind(s);

	// Get identifier
	p = (char)stbi__get8(s);
	t = (char)stbi__get8(s);
	if (p != 'P' || (t != '5' && t != '6'))
	{
		stbi__rewind(s);
		return 0;
	}

	*comp = (t == '6') ? 3 : 1; // '5' is 1-component .pgm; '6' is 3-component .ppm

	c = (char)stbi__get8(s);
	stbi__pnm_skip_whitespace(s, &c);

	*x = stbi__pnm_getinteger(s, &c); // read width
	stbi__pnm_skip_whitespace(s, &c);

	*y = stbi__pnm_getinteger(s, &c); // read height
	stbi__pnm_skip_whitespace(s, &c);

	maxv = stbi__pnm_getinteger(s, &c); // read max value

	if (maxv > 255)
		return stbi__err("max value > 255", "PPM image not 8-bit");
	else
		return 1;
}
#endif
