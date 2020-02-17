module main

fn print_primes_to(top int)
{
	mut primes := []int
	
    top := int32_sqrt(top);
	for (mut totry = 3: totry < top; totry += 2) {
        mut found := false
		for (value in primes) {
			if (totry % value == 0) {
                found = true
				break;
			}
		}
		if (!found) {
			primes << totry;
		}
	}
}
