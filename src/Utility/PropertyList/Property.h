#pragma once

class Property
{
public:
	enum class Type
	{
		Bool,
		Int,
		Float,
		String,
		Flag, // The 'flag' property type mimics a boolean property that is always true
		UInt
	};

	typedef variant<bool, int, double, unsigned, string> Value;

	Property(Type type = Type::Bool); // Default property type is bool
	Property(const Property& copy) : type_{ copy.type_ }, value_{ copy.value_ }, has_value_{ copy.has_value_ } {}
	Property(bool value) : type_{ Type::Bool }, value_{ value }, has_value_{ true } {}
	Property(int value) : type_{ Type::Int }, value_{ value }, has_value_{ true } {}
	Property(double value) : type_{ Type::Float }, value_{ value }, has_value_{ true } {}
	Property(const string& value) : type_{ Type::String }, value_{ value }, has_value_{ true } {}
	Property(unsigned value) : type_{ Type::UInt }, value_{ value }, has_value_{ true } {}
	~Property() = default;

	Type type() const { return type_; }
	bool isType(Type type) const { return this->type_ == type; }
	bool hasValue() const { return has_value_; }

	void changeType(Type newtype);
	void setHasValue(bool hv) { has_value_ = hv; }

	operator bool() const { return boolValue(); }
	operator int() const { return intValue(); }
	operator float() const { return (float)floatValue(); }
	operator double() const { return floatValue(); }
	operator string() const { return stringValue(); }
	operator unsigned() const { return unsignedValue(); }

	Property& operator=(bool val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(int val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(float val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(double val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(const string& val)
	{
		setValue(val);
		return *this;
	}

	Property& operator=(unsigned val)
	{
		setValue(val);
		return *this;
	}

	bool          boolValue(bool warn_wrong_type = false) const;
	int           intValue(bool warn_wrong_type = false) const;
	double        floatValue(bool warn_wrong_type = false) const;
	string        stringValue(bool warn_wrong_type = false) const;
	const string& stringValueRef() const;
	unsigned      unsignedValue(bool warn_wrong_type = false) const;

	void setValue(bool val);
	void setValue(int val);
	void setValue(double val);
	void setValue(const string& val);
	void setValue(unsigned val);

	string typeString() const;

private:
	Type  type_;
	Value value_;
	bool  has_value_;
};
