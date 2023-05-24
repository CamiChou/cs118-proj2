setup:
	docker compose up -d

shell:
	docker compose exec node1 bash

clean:
	docker compose down -v --rmi all --remove-orphans

test: 
	python3 grader/executor.py project/server scenarios/setting1.json

test2:
	python3 grader/executor.py project/server scenarios/setting2.json

out:
	cat stdout.txt

err:
	cat stderr.txt