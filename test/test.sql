/*****************************
 *
 * NGramCharTokenizer User Defined Functions
 *
 * Copyright DingQiang Liu(dingqiangliu@gmail.com), 2012 - 2024
 */


-- unit tests for UDTF
select NGramCharTokenizer(NULL) as token;
select 
    error_on_check_false(
        count(*) = 0
        , 'No token of NULL', 'Case: no token of NULL', ''
        ) as "Case: no token of NULL"
from (
    select NGramCharTokenizer(NULL) as token
    ) t;


select NGramCharTokenizer('') as token;
select 
    error_on_check_false(
        count(*) = 0
        , 'No token of empty string', 'Case: no token of empty string', ''
        ) as "Case: no token of empty string"
from (
    select NGramCharTokenizer('') as token
    ) t;


select NGramCharTokenizer('ABC' using parameters minlen=4, maxlen=4) as token;
select 
    error_on_check_false(
        count(*) = 0
        , 'No token of too short string', 'Case: no token of too short string', ''
        ) as "Case: no token of too short string"
from (
    select NGramCharTokenizer('ABC' using parameters minlen=4, maxlen=4) as token
    ) t;


select NGramCharTokenizer('ABC' using parameters minlen=1, maxlen=1) as token;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'A,B,C'
        , 'unigram', 'Case: unigram', ''
        ) as "Case: unigram"
from (
    select NGramCharTokenizer('ABC' using parameters minlen=1, maxlen=1) as token
    ) t;



select NGramCharTokenizer('ABC' using parameters minlen=1, maxlen=2) as token;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'A,AB,B,BC,C'
        , '1-2 grams', 'Case: 1-2 grams', ''
        ) as "Case: 1-2 grams"
from (
    select NGramCharTokenizer('ABC' using parameters minlen=1, maxlen=2) as token
    ) t;


select NGramCharTokenizer('ABC' using parameters minlen=2, maxlen=2) as token;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'AB,BC'
        , 'bigram', 'Case: bigram', ''
        ) as "Case: bigram"
from (
    select NGramCharTokenizer('ABC' using parameters minlen=2, maxlen=2) as token
    ) t;


select NGramCharTokenizer('ABC' using parameters minlen=3, maxlen=3) as token;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'ABC'
        , 'trigram', 'Case: trigram', ''
        ) as "Case: trigram"
from (
    select NGramCharTokenizer('ABC' using parameters minlen=3, maxlen=3) as token
    ) t;


select NGramCharTokenizer('您好Hello' using parameters minlen=3, maxlen=3) as token;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'Hel,ell,llo,好He,您好H'
        , 'multilingual strings', 'Case: multilingual strings', ''
        ) as "Case: multilingual strings"
from (
    select NGramCharTokenizer('您好Hello' using parameters minlen=3, maxlen=3) as token
    ) t;


-- unit tests for text index

-- text index with customized tokenizer
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
create table test_textindex(id int primary key, content varchar(20), author varchar(15), no int) order by id segmented by hash(id) all nodes ksafe;

-- simple text index
create text index test_textindex_index on test_textindex(id, content) stemmer none tokenizer One2TwoNgramTokenizer(varchar(20));

insert into test_textindex values(1, 'ABC', 'DQ', 50);
commit;


select * from test_textindex_index;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'A,AB,B,BC,C'
        , 'text index', 'Case: text index', ''
        ) as "Case: text index"
from (
    select token from test_textindex_index
    ) t;


-- text index with additional columns to avoid join
create text index test_textindex_index_additional on test_textindex(id, content, content, author, no) stemmer none tokenizer One2TwoNgramTokenizer(varchar(20));

select * from test_textindex_index_additional;
select 
    error_on_check_false(
        listagg(token) within group(order by token) = 'A,AB,B,BC,C'
        , 'text index', 'Case: text index with additional columns', ''
        ) as "Case: text index with additional columns"
from (
    select token from test_textindex_index_additional
    ) t;

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

