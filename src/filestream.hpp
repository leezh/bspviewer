#ifndef FILESTREAM_HPP
#define FILESTREAM_HPP

#include <SFML/System/InputStream.hpp>
#include <physfs.h>
#include <string>

class FileStream : public sf::InputStream
{
public:
	FileStream(const std::string& path);
	~FileStream();

	bool isOpen();

	sf::Int64 read(void* data, sf::Int64 size);

	sf::Int64 seek(sf::Int64 position);

	sf::Int64 tell();

	sf::Int64 getSize();

private:
	PHYSFS_File* file;
};

#endif // FILESTREAM_HPP
