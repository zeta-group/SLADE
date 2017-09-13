
#ifndef __TRANSLATION_H__
#define __TRANSLATION_H__

#define	TRANS_PALETTE	1
#define TRANS_COLOUR	2
#define TRANS_DESAT		3
#define TRANS_BLEND		4
#define TRANS_TINT		5
#define TRANS_SPECIAL	6

enum SpecialBlend
{
	BLEND_ICE = 0,
	BLEND_DESAT_FIRST = 1,
	BLEND_DESAT_LAST = 31,
	BLEND_INVERSE,
	BLEND_RED,
	BLEND_GREEN,
	BLEND_BLUE,
	BLEND_GOLD,
	BLEND_INVALID,
};

class Translation;

class TransRange
{
	friend class Translation;
public:
	TransRange(uint8_t type)
	{
		this->type_ = type;
		this->o_start_ = 0;
		this->o_end_ = 0;
	}

	virtual ~TransRange() {}

	uint8_t type() const { return type_; }
	uint8_t	oStart() const { return o_start_; }
	uint8_t oEnd() const { return o_end_; }

	void	setOStart(uint8_t val) { o_start_ = val; }
	void	setOEnd(uint8_t val) { o_end_ = val; }

	virtual string	asText() { return ""; }

protected:
	uint8_t	type_;
	uint8_t	o_start_;
	uint8_t	o_end_;
};

class TransRangePalette : public TransRange
{
	friend class Translation;
public:
	TransRangePalette() : TransRange(TRANS_PALETTE) { d_start_ = d_end_ = 0; }
	TransRangePalette(TransRangePalette* copy) : TransRange(TRANS_PALETTE)
	{
		o_start_ = copy->o_start_;
		o_end_ = copy->o_end_;
		d_start_ = copy->d_start_;
		d_end_ = copy->d_end_;
	}

	uint8_t	dStart() const { return d_start_; }
	uint8_t	dEnd() const { return d_end_; }

	void	setDStart(uint8_t val) { d_start_ = val; }
	void	setDEnd(uint8_t val) { d_end_ = val; }

	string	asText() override { return S_FMT("%d:%d=%d:%d", o_start_, o_end_, d_start_, d_end_); }

private:
	uint8_t	d_start_;
	uint8_t	d_end_;
};

class TransRangeColour : public TransRange
{
	friend class Translation;
public:
	TransRangeColour() : TransRange(TRANS_COLOUR) { d_start_ = COL_BLACK; d_end_ = COL_WHITE; }
	TransRangeColour(TransRangeColour* copy) : TransRange(TRANS_COLOUR)
	{
		o_start_ = copy->o_start_;
		o_end_ = copy->o_end_;
		d_start_.set(copy->d_start_);
		d_end_.set(copy->d_end_);
	}

	rgba_t	dStart() const { return d_start_; }
	rgba_t	dEnd() const { return d_end_; }

	void	setDStart(rgba_t col) { d_start_.set(col); }
	void	setDEnd(rgba_t col) { d_end_.set(col); }

	string asText() override
	{
		return S_FMT("%d:%d=[%d,%d,%d]:[%d,%d,%d]",
		             o_start_, o_end_,
		             d_start_.r, d_start_.g, d_start_.b,
		             d_end_.r, d_end_.g, d_end_.b);
	}

private:
	rgba_t d_start_;
	rgba_t d_end_;
};

class TransRangeDesat : public TransRange
{
	friend class Translation;
public:
	TransRangeDesat() : TransRange(TRANS_DESAT) { d_sr_ = d_sg_ = d_sb_ = 0; d_er_ = d_eg_ = d_eb_ = 2; }
	TransRangeDesat(TransRangeDesat* copy) : TransRange(TRANS_DESAT)
	{
		o_start_ = copy->o_start_;
		o_end_ = copy->o_end_;
		d_sr_ = copy->d_sr_;
		d_sg_ = copy->d_sg_;
		d_sb_ = copy->d_sb_;
		d_er_ = copy->d_er_;
		d_eg_ = copy->d_eg_;
		d_eb_ = copy->d_eb_;
	}

	float dSr() const { return d_sr_; }
	float dSg() const { return d_sg_; }
	float dSb() const { return d_sb_; }
	float dEr() const { return d_er_; }
	float dEg() const { return d_eg_; }
	float dEb() const { return d_eb_; }

	void	setDStart(float r, float g, float b) { d_sr_ = r; d_sg_ = g; d_sb_ = b; }
	void	setDEnd(float r, float g, float b) { d_er_ = r; d_eg_ = g; d_eb_ = b; }

	string asText() override
	{
		return S_FMT("%d:%d=%%[%1.2f,%1.2f,%1.2f]:[%1.2f,%1.2f,%1.2f]",
		             o_start_, o_end_, d_sr_, d_sg_, d_sb_, d_er_, d_eg_, d_eb_);
	}

private:
	float d_sr_, d_sg_, d_sb_;
	float d_er_, d_eg_, d_eb_;
};

class TransRangeBlend : public TransRange
{
	friend class Translation;
public:
	TransRangeBlend() : TransRange(TRANS_BLEND) { col_ = COL_RED; }
	TransRangeBlend(TransRangeBlend* copy) : TransRange(TRANS_BLEND)
	{
		o_start_ = copy->o_start_;
		o_end_ = copy->o_end_;
		col_ = copy->col_;
	}

	rgba_t	colour() const { return col_; }
	void	setColour(rgba_t c) { col_ = c; }

	string asText() override
	{
		return S_FMT("%d:%d=#[%d,%d,%d]",
			o_start_, o_end_, col_.r, col_.g, col_.b);
	}

private:
	rgba_t col_;
};

class TransRangeTint : public TransRange
{
	friend class Translation;
public:
	TransRangeTint() : TransRange(TRANS_TINT) { col_ = COL_RED; amount_ = 50; }
	TransRangeTint(TransRangeTint* copy) : TransRange(TRANS_TINT)
	{
		o_start_ = copy->o_start_;
		o_end_ = copy->o_end_;
		col_ = copy->col_;
		amount_ = copy->amount_;
	}

	rgba_t	colour() const { return col_; }
	uint8_t	amount() const { return amount_; }
	void	setColour(rgba_t c) { col_ = c; }
	void	setAmount(uint8_t a) { amount_ = a; }

	string asText() override
	{
		return S_FMT("%d:%d=@%d[%d,%d,%d]",
			o_start_, o_end_, amount_, col_.r, col_.g, col_.b);
	}

private:
	rgba_t	col_;
	uint8_t	amount_;
};

class TransRangeSpecial : public TransRange
{
	friend class Translation;
public:
	TransRangeSpecial() : TransRange(TRANS_SPECIAL) { special_ = ""; }
	TransRangeSpecial(TransRangeSpecial* copy) : TransRange(TRANS_SPECIAL)
	{
		o_start_ = copy->o_start_;
		o_end_ = copy->o_end_;
		special_ = copy->special_;
	}

	string	special() const { return special_; }
	void	setSpecial(string sp) { special_ = sp; }

	string asText() override
	{
		return S_FMT("%d:%d=$%s", o_start_, o_end_, special_);
	}

private:
	string special_;
};

class Palette;
class Translation
{
public:
	Translation();
	~Translation();

	const vector<TransRange*>&	ranges() const { return translations_; }

	void	parse(string def);
	void	parseRange(string range);
	void	read(const uint8_t * data);
	string	asText();
	void	clear();
	void	copy(Translation& copy);
	bool	isEmpty() const { return built_in_name_.IsEmpty() && translations_.size() == 0; }

	unsigned	nRanges() const { return translations_.size(); }
	TransRange*	getRange(unsigned index);
	string		builtInName() const { return built_in_name_; }
	void		setDesaturationAmount(uint8_t amount) { desat_amount_ = amount; }

	rgba_t	translate(rgba_t col, Palette* pal = nullptr);
	rgba_t	specialBlend(rgba_t col, uint8_t type, Palette* pal = nullptr);

	void	addRange(int type, int pos);
	void	removeRange(int pos);
	void	swapRanges(int pos1, int pos2);

	static string getPredefined(string def);

private:
	vector<TransRange*>	translations_;
	string				built_in_name_;
	uint8_t				desat_amount_;
};

#endif//__TRANSLATION_H__
