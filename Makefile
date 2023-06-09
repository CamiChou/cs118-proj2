setup:
	docker compose up -d

shell:
	docker compose exec node1 bash

clean:
	docker compose down -v --rmi all --remove-orphans

test: 
	python3 grader/executor.py project/server scenarios/setting1.json

check:
	python3 grader/packet_generate.py < scenarios/setting1.json

test2:
	python3 grader/executor.py project/server scenarios/setting2.json

check2:
	python3 grader/packet_generate.py < scenarios/setting2.json

test3:
	python3 grader/executor.py project/server scenarios/setting3.json

check3:
	python3 grader/packet_generate.py < scenarios/setting3.json

test4:
	python3 grader/executor.py project/server scenarios/setting4.json

check4:
	python3 grader/packet_generate.py < scenarios/setting4.json

testAll:
	python3 grader/executor.py project/server scenarios/setting1.json
	python3 grader/executor.py project/server scenarios/setting2.json
	python3 grader/executor.py project/server scenarios/setting3.json
	python3 grader/executor.py project/server scenarios/setting4.json

out:
	cat stdout.txt

err:
	cat stderr.txt