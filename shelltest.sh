./tester
./tester -w traces/simple-input > my-simple-output
./tester -w traces/linear-input > my-linear-output
./tester -w traces/random-input > my-random-output
diff -u my-simple-output traces/simple-expected-output
diff -u my-linear-output traces/linear-expected-output
diff -u my-random-output traces/random-expected-output
