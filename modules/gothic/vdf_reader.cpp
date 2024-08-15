/**************************************************************************/
/*  vdf_reader.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "vdf_reader.h"

#include "core/os/memory.h"

namespace {
extern "C" const PHYSFS_Archiver __PHYSFS_Archiver_VDF;

PHYSFS_sint64 file_access_read(PHYSFS_Io *io, void *buf, PHYSFS_uint64 len) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(io->opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return static_cast<PHYSFS_sint64>((*fa)->get_buffer(static_cast<uint8_t *>(buf), len));
}

int file_access_seek(PHYSFS_Io *io, PHYSFS_uint64 offset) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(io->opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	(*fa)->seek(offset);
	return 1;
}

PHYSFS_sint64 file_access_tell(PHYSFS_Io *io) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(io->opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return static_cast<PHYSFS_sint64>((*fa)->get_position());
}

PHYSFS_sint64 file_access_length(PHYSFS_Io *io) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(io->opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return static_cast<PHYSFS_sint64>((*fa)->get_length());
}

PHYSFS_Io *file_access_duplicate(PHYSFS_Io *io) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(io->opaque);
	ERR_FAIL_NULL_V(fa, nullptr);
	ERR_FAIL_COND_V(fa->is_null(), nullptr);

	PHYSFS_Io *copy = static_cast<PHYSFS_Io *>(memalloc(sizeof(PHYSFS_Io)));
	memcpy(copy, io, sizeof(PHYSFS_Io));
	(*fa)->reference();
	return copy;
}

void file_access_destroy(struct PHYSFS_Io *io) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(io->opaque);
	if (fa == nullptr || fa->is_null()) {
		return;
	}

	fa->unref();
	memfree(io);
}

PHYSFS_Io *file_access_make_io(Ref<FileAccess> *fa) {
	PHYSFS_Io *io = static_cast<PHYSFS_Io *>(memalloc(sizeof(PHYSFS_Io)));
	io->version = 0;
	io->opaque = fa;
	io->read = &::file_access_read;
	io->write = nullptr;
	io->seek = &::file_access_seek;
	io->tell = &::file_access_tell;
	io->length = &::file_access_length;
	io->duplicate = &::file_access_duplicate;
	io->flush = nullptr;
	io->destroy = &::file_access_destroy;
	return io;
}
} //namespace

Error VdfReader::open(const String &p_path) {
	if (fa.is_valid()) {
		close();
	}

	fa = FileAccess::open(p_path, FileAccess::READ);
	io = ::file_access_make_io(&fa);
	int claimed = 0;
	archive = __PHYSFS_Archiver_VDF.openArchive(io, p_path.utf8().get_data(), 0, &claimed);
	if (archive == nullptr) {
		close();
		return FAILED;
	}

	return OK;
}

Error VdfReader::close() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "VdfReader cannot be closed because it is not open.");

	if (archive != nullptr) {
		__PHYSFS_Archiver_VDF.closeArchive(archive);
		archive = nullptr;
		io = nullptr;
	}

	if (io != nullptr) {
		// this happens if the archive could not be opened, otherwise the io is already released above
		io->destroy(io);
		io = nullptr;
	}

	DEV_ASSERT(fa == nullptr);

	return OK;
}

PackedStringArray VdfReader::get_files() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedStringArray(), "VdfReader must be opened before use.");

	PackedStringArray arr;
	__PHYSFS_Archiver_VDF.enumerate(archive, "", [](void *data, const char *orig_dir, const char *file_name) -> PHYSFS_EnumerateCallbackResult {
		static_cast<PackedStringArray*>(data)->append(String::utf8(file_name));
		return PHYSFS_ENUM_OK; }, "", &arr);
	return arr;
}

VdfReader::VdfReader() {}

VdfReader::~VdfReader() {
	if (fa.is_valid()) {
		close();
	}
}

void VdfReader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open", "path"), &VdfReader::open);
	ClassDB::bind_method(D_METHOD("close"), &VdfReader::close);
	ClassDB::bind_method(D_METHOD("get_files"), &VdfReader::get_files);
}
