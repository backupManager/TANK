#include "tank_client.h"

int main(int argc, char *argv[])
{
        TankClient client;
        const strwlen32_t req(argv[1]);

        if (req.Eq(_S("get")))
        {
                client.consume(
                    {{{"bp_activity", 0}, {argc > 2 ? strwlen32_t(argv[2]).AsUint32() : UINT64_MAX, 10'000}}}, 10000, 128);
        }
        else if (req.Eq(_S("set")))
        {
		if (argc > 2)
		{
			std::vector<TankClient::msg> msgs;
			IOBuffer b;
			const auto now = Timings::Milliseconds::SysTime();
			int fd = open(argv[2], O_RDONLY|O_LARGEFILE);

			Print("Reading ", argv[2], " ..\n");

			if (fd == -1)
			{
				Print("Unable to read ", argv[2], "\n");
				return 1;
			}

			const auto fileSize = lseek64(fd, 0, SEEK_END);

			if (!fileSize)
			{
				close(fd);
				Print("Empty file\n");
				return 1;
			}

			auto fileData = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd, 0);

			assert(fileData != MAP_FAILED);
			madvise(fileData, fileSize, MADV_SEQUENTIAL);




			Print("Processing\n");
                        for (const auto &line : strwlen32_t((char *)fileData, fileSize).Split('\n'))
                        {
                                msgs.push_back({line, now});

                                if (msgs.size() == 128)
                                {
                                        client.produce(
                                            {{{"bp_activity", 0},
                                              msgs}

                                            });

                                        msgs.clear();
                                }
                        }

                        if (msgs.size())
                        {
                                client.produce(
                                    {{{"bp_activity", 0},
                                      msgs}

                                    });
                        }

			munmap(fileData, fileSize);
                }
		else
                {
                        client.produce(
                            {{{"bp_activity", 0},
                              {{"world of warcraft GAME", Timings::Milliseconds::SysTime()},
                               {"Amiga 1200 Computer", Timings::Milliseconds::SysTime(), "computer"},
                               {"lord of the rings, the return of the king", Timings::Milliseconds::SysTime()}}}});
                }
        }

	try
	{
                for (;;)
                {
                        client.poll(1000);

                        for (const auto &it : client.consumed())
                        {
                                Print("Consumed from ", it.topic, ".", it.partition, ", next {", it.next.seqNum, ", ", it.next.minFetchSize, "}\n");

                                for (const auto mit : it.msgs)
                                {
                                        Print("[", mit->key, "]:", mit->content, "(", mit->seqNum, ")\n");
                                }
                        }

                        for (const auto &it : client.faults())
                        {
                                Print("Fault for ", it.clientReqId, "\n");
                                switch (it.type)
                                {
                                        case TankClient::fault::Type::BoundaryCheck:
                                                Print("firstAvailSeqNum = ", it.ctx.firstAvailSeqNum, ", hwMark = ", it.ctx.highWaterMark, "\n");
                                                break;

                                        default:
                                                break;
                                }
                        }

                        for (const auto &it : client.produce_acks())
                        {
                                Print("Produced OK ", it.clientReqId, "\n");
                        }
                }
        }
        catch (const std::exception &e)
        {
		Print("failed:", e.what(), "\n");
        }

        return 0;
}
