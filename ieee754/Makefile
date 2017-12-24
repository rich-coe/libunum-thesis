PROBLEMS = devil bank
LMPROBLEMS = spike

all: $(PROBLEMS) $(LMPROBLEMS)

$(LMPROBLEMS): LDFLAGS = -lm

%: %.c
	cc $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(PROBLEMS) $(LMPROBLEMS)
