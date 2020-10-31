#include "files.h"

size_t
shim_file_size (Shim_File_t const file)
{
#if    defined (SHIM_OS_UNIXLIKE)
	struct stat stat_st;
	if( fstat( file, &stat_st ) == -1 )
		SHIM_ERRX ("Error: Unable to fstat file descriptor #%d\n", file);
	return (size_t)stat_st.st_size;
#elif  defined (SHIM_OS_WINDOWS)
	LARGE_INTEGER lg_int;
	if( GetFileSizeEx( file, &lg_int ) == 0 )
		SHIM_ERRX ("Error: GetFileSizeEx() failed\n");
	return (size_t)lg_int.QuadPart;
#else
#	error "Unsupported operating system."
#endif // ~ #if defined (SHIM_OS_UNIXLIKE) ...
}

size_t
shim_filepath_size (char const * filepath) {
#if    defined (SHIM_OS_UNIXLIKE)
	struct stat stat_st;
	if( stat( filepath, &stat_st ) == -1 )
		SHIM_ERRX ("Error: Unable to stat filepath %s. Does the file exist?\n", filepath);
	return (size_t)stat_st.st_size;
#else /* Any other OS */
	Shim_File_t file = shim_open_existing_filepath( filepath, true );
	size_t const size = shim_file_size( file );
	shim_close_file( file );
	return size;
#endif
}

bool
shim_filepath_exists (char const * filepath)
{
	bool exists = false;
	FILE * test = fopen( filepath, "rb" );
	if( test ) {
		fclose( test );
		exists = true;
	}
	return exists;
}

void
shim_enforce_filepath_existence (char const * SHIM_RESTRICT filepath,
				 bool const                 force_to_exist)
{
	if( shim_filepath_exists( filepath ) ) {
		if( !force_to_exist )
			SHIM_ERRX ("Error: The file %s seems to already exist.\n", filepath);
	} else {
		if( force_to_exist )
			SHIM_ERRX ("Error: The file %s does not seem to exist.\n", filepath);
	}
}

Shim_File_t
shim_open_existing_filepath (char const * SHIM_RESTRICT filepath,
			     bool const                 readonly)
{
	shim_enforce_filepath_existence( filepath, true );
	Shim_File_t file;
#if    defined (SHIM_OS_UNIXLIKE)
	int const read_write_rights = (readonly ? O_RDONLY : O_RDWR);
	if( (file = open( filepath, read_write_rights, (mode_t)0600 )) == -1 )
		SHIM_ERRX ("Error: Unable to open existing file %s with open()\n", filepath);
#elif  defined (SHIM_OS_WINDOWS)
	DWORD read_write_rights = GENERIC_READ;
	if( readonly )
		read_write_rights |= GENERIC_WRITE;
	if( (file = CreateFileA( filepath, read_write_rights, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
		SHIM_ERRX ("Error: Unable to open existing file %s with CreateFileA()\n", filepath);
#else
#	error "Unsupported operating system."
#endif
	return file;
}

Shim_File_t
shim_create_filepath (char const *filepath)
{
	shim_enforce_filepath_existence( filepath, false );
	Shim_File_t file;
#if    defined (SHIM_OS_UNIXLIKE)
	if( (file = open( filepath, O_RDWR|O_TRUNC|O_CREAT, (mode_t)0600 )) == -1 )
		SHIM_ERRX ("Error: Unable to create new file %s with open()\n", filepath);
#elif  defined (SHIM_OS_WINDOWS)
	if( (file = CreateFileA( filepath, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
		SHIM_ERRX ("Error: Unable to create file %s with CreateFileA()\n", filepath);
#else
#	error "Unsupported operating system."
#endif
	return file;
}

void
shim_close_file (Shim_File_t const file)
{
#if    defined (SHIM_OS_UNIXLIKE)
	if( close( file ) == -1 )
		SHIM_ERRX ("Error: Wasn't able to close file descriptor %d\n", file);
#elif  defined (SHIM_OS_WINDOWS)
	if( CloseHandle( file ) == 0 )
		SHIM_ERRX ("Error: Wasn't able to close file handle\n");
#else
#	error "Unsupported operating system."
#endif
}

void
shim_set_file_size (Shim_File_t const file, size_t const new_size)
{
#if    defined (SHIM_OS_UNIXLIKE)
	if( ftruncate( file, new_size ) == -1 )
		SHIM_ERRX ("Error: Failed to set size of file descriptor %d to %zu\n", file, new_size);
#elif  defined (SHIM_OS_WINDOWS)
	LARGE_INTEGER lg_int;
	lg_int.QuadPart = new_size;
	if( SetFilePointerEx( file, lg_int, NULL, FILE_BEGIN ) == 0 )
		SHIM_ERRX ("Error: Failed to SetFilePointerEx()\n");
	if( SetEndOfFile( file ) == 0 )
		SHIM_ERRX ("Error: Failed to SetEndOfFile()\n");
#else
#	error "Unsupported operating system."
#endif
}
