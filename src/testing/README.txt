1. For funcitonal testing:
    - cd into the exchange-matching directory
    - run "sudo docker-compose up"
    - open another terminal
    - cd into testing directory
    - run "make"
    - run "./functionality-test-client"

2. For scalability testing:
    - if you just finished the funcitonal testing, kill the server
    - cd into the exchange-matching directory
    - change the name of docker-compose-scalability.yml to docker-compose.yml; docker-compose.yml to anything you like
    - run "sudo docker-compose up"
    - open another terminal
    - cd into testing directory
    - run "make"
    - run "./scalability-test-client 10000"


Tips for scalability test:
    The default settings for the server is of thread pool size 64, 10000 client requests, and 4 CPU cores.
    To change it, just edit the docker-compose.yml file: change this line "./scalability-test-server 64 10000".
    Remember to keep client requests number the same as the third parameter of the server