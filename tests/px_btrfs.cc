#include <stdlib.h>
#include <gtest/gtest.h>
//#include <sys/stat.h>
#include <fcntl.h>
//#include <linux/fuse.h>
//#include <sys/poll.h>
//#include <sys/uio.h>
#include <string>
//#include <boost/lexical_cast.hpp>
#include <functional>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace std::placeholders;

class PXBtrfs : public ::testing::Test {
protected:
	int fd;		// control file descriptor
	std::set<uint64_t> added_ids;
	const size_t write_len = 4096;
	std::atomic<uint64_t> wr_thr_cnt;

	PXBtrfs() : fd(-1), wr_thr_cnt(0) {}
	virtual ~PXBtrfs() {
		if (fd >= 0)
			close(fd);
	}

	virtual void SetUp();
	virtual void TearDown();

public:
	void write_thread(const char *name);
	void read_thread(const char *name);
};

void PXBtrfs::SetUp()
{
	seteuid(0);
	ASSERT_EQ(0, system("/usr/bin/sudo /usr/sbin/insmod btrfs.ko"));
}

void PXBtrfs::TearDown()
{
	sleep(1);

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	ASSERT_EQ(0, system("/usr/bin/sudo /usr/sbin/rmmod btrfs.ko"));
}

static std::vector<uint64_t> make_pattern(size_t size)
{
	std::vector<uint64_t> v(size / sizeof(uint64_t));
	for (size_t i = 0; i < v.size(); ++i)
		v[i] = i;
	return v;
}

void PXBtrfs::write_thread(const char *name)
{
	for (int i = 0; i < 1000000; i++) {
		off_t offset = rand() % (512 * 1024);
		size_t sz = rand() % (16 * 1024);

		if (sz <= 16) {
			sz = 100;
		}

		std::vector<uint64_t> v(make_pattern(sz));
		int fd = -1;

		offset = 8192 + (offset & ~4095);

		fprintf(stderr, "\n\n\nITTR %d Starting IO at offset %lu with size %lu\n", i, offset, sz);

		ASSERT_EQ(offset, lseek(fd, offset, SEEK_SET));
		ssize_t write_bytes = write(fd, v.data(), sz);
		if (write_bytes == -1) {
			perror("write_thread");
		}
		ASSERT_EQ(write_bytes, sz);

		ASSERT_EQ(offset - 8192, lseek(fd, offset - 8192, SEEK_SET));
		write_bytes = write(fd, v.data(), sz);
		if (write_bytes == -1) {
			perror("write_thread");
		}

		ASSERT_TRUE(write_bytes == (int)sz || errno == EINTR);

		ASSERT_EQ(offset + 8192, lseek(fd, offset + 8192, SEEK_SET));
		write_bytes = write(fd, v.data(), sz);
		if (write_bytes == -1) {
			perror("write_thread");
		}
		ASSERT_EQ(write_bytes, sz);

		fprintf(stderr, "Wrote %lu bytes at %lu\n", sz, offset);
	}

	uint64_t th = wr_thr_cnt;
	fprintf(stderr, "THREAD %lu exiting\n", th);
	wr_thr_cnt++;
}

TEST_F(PXBtrfs, read_write)
{
	std::string name;
	char msg_buf[write_len * 8 * 2];

	std::thread writer1(&PXBtrfs::write_thread, this, name.c_str());
	std::thread writer2(&PXBtrfs::write_thread, this, name.c_str());

	while (1) {
		ssize_t write_bytes = write(fd, msg_buf, 4096);
		ASSERT_EQ(4096, write_bytes);
	}

done:
	writer1.join();
	writer2.join();
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
