#pragma once

#include <cdk/ast/literal_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {
/**
 * Class for describing tensor literal nodes.
 * A tensor literal is represented by a sequence of its elements,
 * where elements can be other literals or nested sequences.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the tensor expression appears.
 * - content_sequence: A cdk::sequence_node representing the tensor's elements.
 */
class tensor_node : public cdk::literal_node<cdk::sequence_node*> {
public:
    tensor_node(int lineno, cdk::sequence_node *content_sequence) noexcept
        : cdk::literal_node<cdk::sequence_node*>(lineno, content_sequence) {
    }

    ~tensor_node() {
        delete value();
    }

    cdk::sequence_node* content() const {
        return value();
    }

    void accept(basic_ast_visitor *sp, int level) override {
        sp->do_tensor_node(this, level);
    }
};

} // namespace udf
