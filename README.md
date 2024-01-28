# NGramCharTokenizer for Vertica, convert text to ngram characters.

This is a Vertica User Defined Functions (UDF) named NGramCharTokenizer, a characters ngram tokenizer for text index.

## Syntax

1. **NGramCharTokenizer** ( text [using parameters minLen=... [, maxLen=...] ] )
   Get all possible ngrams with lengh range[minLen, maxLen].

   ***Parameters:***
   
   * text: string.
   * minLen: int, minimum length of NGram, default is 1.
   * maxLen: int, maximum length of NGram, default is 3.
   * (return): list of NGrams.

2. **SetNGramCharTokenizerParameters** (minLen, maxLen using parameters procOid)
   Setting configuation of GramCharTokenizer for text index since there is no chance to provide them in CREATE TEXT INDEX statement.

   ***Parameters:***
   
   * minLen: int, minimum length of NGram.
   * maxLen: int, maximum length of NGram.
   * procOid: string, oid of the specified function, different text index can use different instance of definition with different parameters.
   * (return): bool, success or failure of setting.

## Examples


- work as UDTF:
  
  ```SQL
  SELECT NGramCharTokenizer('ABC' using parameters minlen=1, maxlen=1) as tokens;
   tokens
  --------
   A
   B
   C
  (3 rows)
  
  SELECT NGramCharTokenizer('ABC' using parameters minlen=2, maxlen=2) as tokens;
   tokens
  --------
   AB
   BC
  (2 rows)
  
  SELECT NGramCharTokenizer('ABC' using parameters minlen=3, maxlen=3) as tokens;
   tokens
  --------
   ABC
  (1 row)
  
  SELECT NGramCharTokenizer('您好Hello' using parameters minlen=3, maxlen=3) as tokens;
   tokens
  --------
   您好H
   好He
   Hel
   ell
   llo
  (5 rows)
  ```
  
- work as a tokenizer for text index:
  ```SQL
  drop transform function if exists One2TwoNgramTokenizer();
  create transform function One2TwoNgramTokenizer AS language 'C++' name 'NGramCharTokenizerFactory' library ngramchar fenced;
  do $$
  declare
      procid int;
  begin
      select proc_oid into procid from vs_procedures where procedure_name = 'One2TwoNgramTokenizer';
      execute 'select SetNGramCharTokenizerParameters(1, 2 using parameters proc_oid='''||procid||''')';
  end;
  $$;
  
  drop table if exists test_textindex;
  create table test_textindex(id int primary key, content varchar(20)) order by id segmented by hash(id) all nodes ksafe;
  create text index test_textindex_index on test_textindex(id, content) stemmer none tokenizer One2TwoNgramTokenizer(varchar(20));
  
  insert into test_textindex values(1, 'ABC');
  commit;
  
  
  select * from test_textindex_index;
   token | doc_id
  -------+--------
   A     |      1
   AB    |      1
   B     |      1
   BC    |      1
   C     |      1
  (5 rows)
  
  
  drop text index test_textindex_index;
  drop table if exists test_textindex cascade;
  do $$
  declare
      procid int;
  begin
      select proc_oid into procid from vs_procedures where procedure_name = 'One2TwoNgramTokenizer';
      execute 'select dfs_delete(''/ngramTokenizersConfigurations/'||procid||''', true)';
  end;
  $$;
  drop transform function if exists One2TwoNgramTokenizer();
  ```

## Install, test and uninstall

Before build and install, g++ should be available(**yum -y groupinstall "Development tools" && yum -y groupinstall "Additional Development"** can help on this).

 * Build: 

   ```bash
   autoreconf -f -i && ./configure && make
   ```

 * Install: 

   ```bash
   make install
   ```

 * Test: 

   ```bash
   make run
   ```

 * Uninstall: 

   ```bash
   make uninstall
   ```

 * Clean: 

   ```bash
   make distclean
   ```

 * Clean all: 

   ```bash
   git clean -i .
   ```
