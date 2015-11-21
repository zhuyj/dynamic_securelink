# dynamic_securelink

# github guide

git clone https://zhuyj@github.com/zhuyj/dynamic_securelink.git
cd dynamic_securelink
git remote set-url origin https://zhuyj@github.com/zhuyj/dynamic_securelink.git

Scenario

The scenario is as below.

When there is no needs, the connection to the Another World is shut down. When user1 or user2 expects to access the Another World, he only accesses the web sites directly. When dynamic_securelink detects the packets, it will create the secure connection to some Server. Then it will encrypt the packets, then send it to the Another World by this secure connection.

If no any needs, the dynamic secure connection will be shut down.

        user1 user2
         |     |                    FireWall
+_-------------------+                  |     +--------+
| dynamic_securelink |<----dynamically--|---->| Server |<---Another-World--...
+____________________+                  |     +________+


Limited state machine

FAQ

