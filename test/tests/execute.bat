"../../compiler/bin/sing" "-^" 1 lexer_test.sing > ../tests_results/lexer_test.txt
"../../compiler/bin/sing" "-^" 2 parser_test.sing -o ../tests_results/parser_test.txt
"../../compiler/bin/sing" "-^" 2 parser_errors_test.sing > ../tests_results/parser_errors_test.txt
"../../compiler/bin/sing" "-^" 3 checker_test.sing > ../tests_results/checker_test.txt
"../../compiler/bin/sing" "-^" 3 -I ./ checker_test_classes.sing > ../tests_results/checker_test_classes.txt
"../../compiler/bin/sing" "-^" 3 -I ../tests -I ../ -u checker_pkgs_test.sing > ../tests_results/checker_pkgs_test.txt
"../../compiler/bin/sing" "-^" 3 usage.sing > ../tests_results/usage.txt
"../../compiler/bin/sing" -u formatter_errors.sing -o ../tests_results/formatter_errors.cpp > ../tests_results/formatter_errors.txt
"../../compiler/bin/sing" -u formatter_test.sing -o ../tests_results/formatter_test.cpp
