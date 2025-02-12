#ifndef FILESTREAM_HPP
#define FILESTREAM_HPP

#include <SFML/System/InputStream.hpp>
#include <physfs.h>
#include <string>
#include <optional>

class FileStream : public sf::InputStream
{
public:
	FileStream(const std::string& path);
	~FileStream();
	bool isOpen();
	std::optional<std::size_t> read(void* data, std::size_t size);
    std::optional<std::size_t> seek(std::size_t position);
    std::optional<std::size_t> tell();
    std::optional<std::size_t> getSize();

private:
	PHYSFS_File* file;
};

#endif // FILESTREAM_HPP
