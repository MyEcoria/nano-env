struct bucket{T}
    transactions::ds.SortedSet{transaction{T}}
    max
end

function first(b::bucket)::transaction
    ds.first(b.transactions)
end

function element_type(b::bucket{T}) where{T}
    T
end

function isempty(b::bucket)
    isempty(b.transactions)
end

function length(b::bucket)
    length(b.transactions)
end

function bucket(; type = transaction_type_default, bucket_max = bucket_max_default)
    bucket(ds.SortedSet{transaction{type}}(), bucket_max)
end

function in(transaction, b::bucket)
    transaction ∈ b.transactions
end

function transactions(b::bucket)
    result = Set{transaction{element_type(b)}}(b.transactions)
end

function full(b::bucket)
    length(b.transactions) >= b.max
end

function insert!(b::bucket, transaction)
    if full(b)
        delete!(b.transactions, last(b.transactions))
    end
    insert!(b.transactions, transaction)
end

function load_factor(b::bucket)
    return length(b.transactions) / b.max
end
