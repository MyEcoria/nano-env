

mutable struct stat_struct
    deleted::UInt
    inserted::UInt
end

struct network{T}
    nodes::Vector{node{T}}
    transactions::ds.SortedSet{transaction{T}}
    stats::stat_struct
end

function network(; type = transaction_type_default, node_count = node_count_default, bucket_count = bucket_count_default, bucket_max = bucket_max_default)
    nodes = []
    for i = 0:node_count - 1
        push!(nodes, node(type = type, bucket_count = bucket_count, bucket_max = bucket_max))
    end
    transactions = ds.SortedSet{transaction{type}}()
    network{type}(nodes, transactions, stat_struct(0, 0))
end

function in(transaction, n::network)
    transaction in n.transactions
end

function quorum(n::network)
    ((2 * length(n.nodes)) ÷ 3) + 1
end

function confirmed_set(n::network)
    weights = Dict{transaction, UInt}()
    for node in n.nodes
        s = working_set(node)
        for tx in s
            w = get(weights, tx, 0)
            weights[tx] = w + 1
        end
    end
    result = Set{transaction}()
    for (tx, w) in weights
        if w > quorum(n)
            push!(result, tx)
        end
    end
    result
end

function delete!(n::network, transaction)
    @assert transaction ∈ n.transactions
    delete!(n.transactions, transaction)
    for node in n.nodes
        delete!(node, transaction)
    end
end

function bucket_histogram(n::network)
    result = ds.SortedDict{transaction_type(n), UInt32}()
    for i in n.nodes
        s = sizes(i)
        for (b, l) = s
            result[b] = Base.get(result, b, 0) + l
        end
    end
    result
end

function print(n::network)
    h = bucket_histogram(n)
    print("l:", length(n.transactions), " d:", n.stats.deleted, ' ', h, '\n')
end

function transaction_type(n::network{T}) where{T}
    T
end