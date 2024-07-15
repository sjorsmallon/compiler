enum internal_types
{
	TYPE_INVALID = 0,
	TYPE_U8,
	TYPE_U16,
	TYPE_U32,
	TYPE_U64,
	TYPE_I8,
	TYPE_I16,
	TYPE_I32,
	TYPE_I64,
	TYPE_R32,
	TYPE_R64,
	TYPE_ELEMENT_COUNT
};

const char* internal_type_strings[TYPE_ELEMENT_COUNT] = {
  	[TYPE_INVALID] ="TYPE_ERROR",
	[TYPE_U8] = "u8",
	[TYPE_U16] = "u16",
	[TYPE_U32] = "u32",
	[TYPE_U64] = "u64",
	[TYPE_I8] = "i8",
	[TYPE_I16] = "i16",
	[TYPE_I32] = "i32",
	[TYPE_I64] = "i64",
	[TYPE_R32] = "r32",
	[TYPE_R64] = "r64"
};