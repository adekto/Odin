typedef struct BuildContext {
	String ODIN_OS;      // target operating system
	String ODIN_ARCH;    // target architecture
	String ODIN_ENDIAN;  // target endian
	String ODIN_VENDOR;  // compiler vendor
	String ODIN_VERSION; // compiler version
	String ODIN_ROOT;    // Odin ROOT

	i64    word_size;
	i64    max_align;
	String llc_flags;
	String link_flags;
	bool   is_dll;
} BuildContext;


gb_global BuildContext build_context = {0};



// TODO(bill): OS dependent versions for the BuildContext
// join_path
// is_dir
// is_file
// is_abs_path
// has_subdir

String const WIN32_SEPARATOR_STRING = {cast(u8 *)"\\", 1};
String const NIX_SEPARATOR_STRING   = {cast(u8 *)"/",  1};

#if defined(GB_SYSTEM_WINDOWS)
String odin_root_dir(void) {
	String path = global_module_path;
	Array(wchar_t) path_buf;
	isize len, i;
	gbTempArenaMemory tmp;
	wchar_t *text;

	if (global_module_path_set) {
		return global_module_path;
	}

	array_init_count(&path_buf, heap_allocator(), 300);

	len = 0;
	for (;;) {
		len = GetModuleFileNameW(NULL, &path_buf.e[0], path_buf.count);
		if (len == 0) {
			return make_string(NULL, 0);
		}
		if (len < path_buf.count) {
			break;
		}
		array_resize(&path_buf, 2*path_buf.count + 300);
	}

	tmp = gb_temp_arena_memory_begin(&string_buffer_arena);

	text = gb_alloc_array(string_buffer_allocator, wchar_t, len+1);

	GetModuleFileNameW(NULL, text, len);
	path = string16_to_string(heap_allocator(), make_string16(text, len));
	for (i = path.len-1; i >= 0; i--) {
		u8 c = path.text[i];
		if (c == '/' || c == '\\') {
			break;
		}
		path.len--;
	}

	global_module_path = path;
	global_module_path_set = true;

	gb_temp_arena_memory_end(tmp);

	array_free(&path_buf);

	return path;
}

#elif defined(GB_SYSTEM_OSX)

#include <mach-o/dyld.h>

String odin_root_dir(void) {
	String path = global_module_path;
	Array(char) path_buf;
	isize len, i;
	gbTempArenaMemory tmp;
	wchar_t *text;

	if (global_module_path_set) {
		return global_module_path;
	}

	array_init_count(&path_buf, heap_allocator(), 300);

	len = 0;
	for (;;) {
		int sz = path_buf.count;
		int res = _NSGetExecutablePath(&path_buf.e[0], &sz);
		if(res == 0) {
			len = sz;
			break;
		} else {
			array_resize(&path_buf, sz + 1);
		}
	}


	tmp = gb_temp_arena_memory_begin(&string_buffer_arena);
	text = gb_alloc_array(string_buffer_allocator, u8, len + 1);
	gb_memmove(text, &path_buf.e[0], len);

	path = make_string(text, len);
	for (i = path.len-1; i >= 0; i--) {
		u8 c = path.text[i];
		if (c == '/' || c == '\\') {
			break;
		}
		path.len--;
	}

	global_module_path = path;
	global_module_path_set = true;

	gb_temp_arena_memory_end(tmp);

	// array_free(&path_buf);

	return path;
}
#else
#error Implement system
#endif


#if defined(GB_SYSTEM_WINDOWS)
String path_to_fullpath(gbAllocator a, String s) {
	gbTempArenaMemory tmp = gb_temp_arena_memory_begin(&string_buffer_arena);
	String16 string16 = string_to_string16(string_buffer_allocator, s);
	String result = {0};

	DWORD len = GetFullPathNameW(string16.text, 0, NULL, NULL);
	if (len != 0) {
		wchar_t *text = gb_alloc_array(string_buffer_allocator, wchar_t, len+1);
		GetFullPathNameW(string16.text, len, text, NULL);
		text[len] = 0;
		result = string16_to_string(a, make_string16(text, len));
	}
	gb_temp_arena_memory_end(tmp);
	return result;
}
#elif defined(GB_SYSTEM_OSX) || defined(GB_SYSTEM_UNIX)
String path_to_fullpath(gbAllocator a, String s) {
	char* p = realpath(s.text, 0);
	// GB_ASSERT(p && "file does not exist");
	if(!p) return make_string_c("");

	return make_string(p, strlen(p));
}
#else
#error Implement system
#endif


String get_fullpath_relative(gbAllocator a, String base_dir, String path) {
	String res = {0};
	isize str_len = base_dir.len+path.len;

	u8 *str = gb_alloc_array(heap_allocator(), u8, str_len+1);

	isize i = 0;
	gb_memmove(str+i, base_dir.text, base_dir.len); i += base_dir.len;
	gb_memmove(str+i, path.text, path.len);
	str[str_len] = '\0';
	res = path_to_fullpath(a, make_string(str, str_len));
	gb_free(heap_allocator(), str);
	return res;
}

String get_fullpath_core(gbAllocator a, String path) {
	String module_dir = odin_root_dir();
	String res = {0};

	char core[] = "core/";
	isize core_len = gb_size_of(core)-1;

	isize str_len = module_dir.len + core_len + path.len;
	u8 *str = gb_alloc_array(heap_allocator(), u8, str_len+1);

	gb_memmove(str, module_dir.text, module_dir.len);
	gb_memmove(str+module_dir.len, core, core_len);
	gb_memmove(str+module_dir.len+core_len, path.text, path.len);
	str[str_len] = '\0';

	res = path_to_fullpath(a, make_string(str, str_len));
	gb_free(heap_allocator(), str);
	return res;
}




void init_build_context(void) {
	BuildContext *bc = &build_context;
	bc->ODIN_VENDOR  = str_lit("odin");
	bc->ODIN_VERSION = str_lit("0.1.1");
	bc->ODIN_ROOT    = odin_root_dir();

#if defined(GB_SYSTEM_WINDOWS)
	bc->ODIN_OS      = str_lit("windows");
	bc->ODIN_ARCH    = str_lit("amd64");
	bc->ODIN_ENDIAN  = str_lit("little");
#elif defined(GB_SYSTEM_OSX)
	bc->ODIN_OS      = str_lit("osx");
	bc->ODIN_ARCH    = str_lit("amd64");
	bc->ODIN_ENDIAN  = str_lit("little");
#else
#error Implement system
#endif

	if (str_eq(bc->ODIN_ARCH, str_lit("amd64"))) {
		bc->word_size = 8;
		bc->max_align = 16;
		bc->llc_flags = str_lit("-march=x86-64 ");
		bc->link_flags = str_lit("/machine:x64 ");
	} else if (str_eq(bc->ODIN_ARCH, str_lit("x86"))) {
		bc->word_size = 4;
		bc->max_align = 8;
		bc->llc_flags = str_lit("-march=x86 ");
		bc->link_flags = str_lit("/machine:x86 ");
	}
}
