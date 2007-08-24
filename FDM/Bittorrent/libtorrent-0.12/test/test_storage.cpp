/*
  Free Download Manager Copyright (c) 2003-2007 FreeDownloadManager.ORG
*/

    

#include "libtorrent/storage.hpp"
#include "libtorrent/file_pool.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/aux_/session_impl.hpp"

#include <boost/utility.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/thread/mutex.hpp>

#include "test.hpp"

using namespace libtorrent;
using namespace boost::filesystem;

const int piece_size = 16;

void run_storage_tests(torrent_info& info)
{
	const int half = piece_size / 2;

	char piece0[piece_size] =
	{ 6, 6, 6, 6, 6, 6, 6, 6
	, 9, 9, 9, 9, 9, 9, 9, 9};

	char piece1[piece_size] =
	{ 0, 0, 0, 0, 0, 0, 0, 0
	, 1, 1, 1, 1, 1, 1, 1, 1};

	char piece2[piece_size] =
	{ 0, 0, 1, 0, 0, 0, 0, 0
	, 1, 1, 1, 1, 1, 1, 1, 1};

	info.set_hash(0, hasher(piece0, piece_size).final());
	info.set_hash(1, hasher(piece1, piece_size).final());
	info.set_hash(2, hasher(piece2, piece_size).final());
	
	info.create_torrent();

	create_directory(initial_path() / "temp_storage");

	int num_pieces = (1 + 612 + 17 + piece_size - 1) / piece_size;
	TEST_CHECK(info.num_pieces() == num_pieces);

	char piece[piece_size];

	{ 
	file_pool fp;
	storage s(info, initial_path(), fp);

	
	s.write(piece1, 0, 0, half);
	s.write(piece1 + half, 0, half, half);

	
	TEST_CHECK(s.read(piece, 0, 0, piece_size) == piece_size);
	TEST_CHECK(std::equal(piece, piece + piece_size, piece1));
	
	
	s.write(piece0, 1, 0, piece_size);
	s.write(piece2, 2, 0, piece_size);

	
	TEST_CHECK(s.read(piece, 1, 0, piece_size) == piece_size);
	TEST_CHECK(std::equal(piece, piece + piece_size, piece0));

	s.read(piece, 2, 0, piece_size);
	TEST_CHECK(std::equal(piece, piece + piece_size, piece2));
	
	s.release_files();
	}

	
	{
	file_pool fp;
	piece_manager pm(info, initial_path(), fp);
	boost::mutex lock;
	libtorrent::aux::piece_checker_data d;

	std::vector<bool> pieces;
	num_pieces = 0;
	TEST_CHECK(pm.check_fastresume(d, pieces, num_pieces, true) == false);
	bool finished = false;
	float progress;
	num_pieces = 0;
	boost::recursive_mutex mutex;
	while (!finished)
		boost::tie(finished, progress) = pm.check_files(pieces, num_pieces, mutex);

	TEST_CHECK(num_pieces == std::count(pieces.begin(), pieces.end()
		, true));

	pm.move_storage("temp_storage2");
	TEST_CHECK(!exists("temp_storage"));
	TEST_CHECK(exists("temp_storage2/temp_storage"));
	pm.move_storage(".");
	TEST_CHECK(!exists("temp_storage2/temp_storage"));	
	remove_all("temp_storage2");

	TEST_CHECK(pm.read(piece, 0, 0, piece_size) == piece_size);
	TEST_CHECK(std::equal(piece, piece + piece_size, piece0));

	TEST_CHECK(pm.read(piece, 1, 0, piece_size) == piece_size);
	TEST_CHECK(std::equal(piece, piece + piece_size, piece1));

	TEST_CHECK(pm.read(piece, 2, 0, piece_size) == piece_size);
	TEST_CHECK(std::equal(piece, piece + piece_size, piece2));
	pm.release_files();

	}
}

int test_main()
{
	torrent_info info;
	info.set_piece_size(piece_size);
	info.add_file("temp_storage/test1.tmp", 17);
	info.add_file("temp_storage/test2.tmp", 612);
	info.add_file("temp_storage/test3.tmp", 0);
	info.add_file("temp_storage/test4.tmp", 0);
	info.add_file("temp_storage/test5.tmp", 1);

	run_storage_tests(info);

	
	TEST_CHECK(file_size(initial_path() / "temp_storage" / "test1.tmp") == 17);
	TEST_CHECK(file_size(initial_path() / "temp_storage" / "test2.tmp") == 31);
	TEST_CHECK(exists("temp_storage/test3.tmp"));
	TEST_CHECK(exists("temp_storage/test4.tmp"));
	remove_all(initial_path() / "temp_storage");

	info = torrent_info();
	info.set_piece_size(piece_size);
	info.add_file("temp_storage/test1.tmp", 17 + 612 + 1);

	run_storage_tests(info);

	
	TEST_CHECK(file_size(initial_path() / "temp_storage" / "test1.tmp") == 48);
	remove_all(initial_path() / "temp_storage");

	return 0;
}

