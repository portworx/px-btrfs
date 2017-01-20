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

class PXBtrfsTest : public ::testing::Test {
private:
	const uint64_t size_ = 10 * 1024 * 1024 * 1024UL;
	const char *btrfs_fs = "/var/tmp/btrfs";
	const char *btrfs_mount = "/var/tmp/btrfs_mount/";

protected:
	int fd;		// control file descriptor
	std::set<uint64_t> added_ids;
	const size_t write_len = 4096;
	std::atomic<uint64_t> wr_thr_cnt;

	PXBtrfsTest() : fd(-1), wr_thr_cnt(0) {}
	virtual ~PXBtrfsTest() {
		if (fd >= 0)
			close(fd);
	}

	virtual void SetUp();
	virtual void TearDown();

public:
	void write_thread(const char *name);
	void read_thread(const char *name);
};

void PXBtrfsTest::SetUp()
{
	struct stat st;
	char cmd[256];

	seteuid(0);
	ASSERT_EQ(0, system("/usr/bin/sudo /usr/sbin/insmod btrfs.ko"));

	sprintf(cmd, "umount %s", btrfs_mount);
	if (system(cmd)) {}
	sprintf(cmd, "rm -rf %s", btrfs_mount);
	if (system(cmd)) {}

	ASSERT_EQ(-1, stat(btrfs_mount, &st));
	ASSERT_EQ(errno, ENOENT);
	ASSERT_EQ(0, mkdir(btrfs_mount, 0777));

	int fd = ::open(btrfs_fs, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	ASSERT_TRUE(fd >= 0);
	ASSERT_EQ(0, ftruncate(fd, size_));
	sprintf(cmd, "mkfs.btrfs %s", btrfs_fs);
	ASSERT_EQ(0, system(cmd));

	sprintf(cmd, "mount %s %s", btrfs_fs, btrfs_mount);
	ASSERT_EQ(0, system(cmd));
}

void PXBtrfsTest::TearDown()
{
	char cmd[256];
	sleep(1);

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	sprintf(cmd, "mount %s %s", btrfs_mount);
	ASSERT_EQ(0, system(cmd));
	ASSERT_EQ(0, system("/usr/bin/sudo /usr/sbin/rmmod btrfs.ko"));
}

static std::vector<uint64_t> make_pattern(size_t size)
{
	std::vector<uint64_t> v(size / sizeof(uint64_t));
	for (size_t i = 0; i < v.size(); ++i)
		v[i] = i;
	return v;
}

void PXBtrfsTest::write_thread(const char *name)
{

	char file[256];
	sprintf(file, "%s/f.%s", btrfs_mount, name);
	fprintf(stderr, "Openginfile .......%s\n",file);
	int fd = open(file, O_CREAT|O_RDWR|O_DIRECT|O_SYNC);
	if (fd < 0) {
		perror("write_thread");
	}
	for (int i = 0; i < 1000000; i++) {
		off_t offset = rand() % (512 * 1024);
		size_t sz = rand() % (16 * 1024);

		if (sz <= 16) {
			sz = 100;
		}

		std::vector<uint64_t> v(make_pattern(sz));
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

TEST_F(PXBtrfsTest, read_write)
{
	char msg_buf[write_len * 8 * 2];

	std::thread writer1(&PXBtrfsTest::write_thread, this, "writer1");
	std::thread writer2(&PXBtrfsTest::write_thread, this, "writer2");

	writer1.join();
	writer2.join();
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
